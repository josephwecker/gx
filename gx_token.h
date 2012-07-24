/**
 * Creates tokens that can easily be used in HTTP etc using some form of
 * authentication encryption. That is, the tokens confidentially convey data
 * and can be completely authenticated.
 *
 * Ideally it should cache everything it can. Possibly even cache the
 * clock-tick and reuse by all threads/processes.
 *
 *
 * WARNING: This isn't a standalone header yet. At the moment it requires
 * ae.h and something to be compiled against it, which in turn usually needs
 * either native AES cpu instructions or openssl or something.
 *
 * Basic Composition of the Token
 * ------------------------------
 *
 * ### User defined / entered values ###
 *
 * PAYLOAD ( nB|n*8b) = A struct with any data you want to convey directly with
 *                      the token- such as expiration, client IP address, etc.
 * KEY     (32B|256b) = Shared secret (ideally securly randomly generated)
 *                      known by the token issuer(s) and token authenticator(s).
 * VERSION ( 1B|  8b) = A single userdefined byte that will be attached as
 *                      plaintext to the token- use to know how to handle the
 *                      token even before authenticating/deciphering. It is
 *                      eventually authenticated as part of the token. Keep in
 *                      mind that this (along with the NONCE below) are
 *                      PUBLIC and shouldn't expose any information that can be
 *                      exploited.
 *
 * ### Some additional definitions ###
 * 
 * NONCE   (12B| 96b) = Sent in the clear- must never be reused for same key.
 * TAG     (16B|128b) = Part of AES output. Something roughly equiv to an HMAC
 *                      that authenticates the token, when possible embedded
 *                      directly into the cipher.
 * AAD     ( nB|n*8b) = Additionally Authenticated Data - something that will
 *                      be sent in the clear but authenticated (not including
 *                      the NONCE).
 *
 * ### Composition ###
 * 
 * PAYLOAD, KEY, VERSION  = [given]
 *
 * NONCE                 := cat(4 byte host-ip4-address,
 *                              8 least-sig.-bytes of host's monotonic-clock-tick)
 * PLAINTEXT             := compress+serialize(PAYLOAD)
 * AAD                   := VERSION [that's all at the moment]
 * CIPHERTEXT, TAG       := aes-[mode](KEY, PLAINTEXT, NONCE, AAD)
 *
 * RAW_TOKEN             := cat(VERSION, compress(NONCE), CIPHERTEXT, TAG)
 * TOKEN                 := base64-url-encode(RAW_TOKEN)
 *
 * ### Discussion ###
 *
 * The size will be roughly (in bytes):
 *  (1 + 12 + [PLAINTEXT-size] + 16) * 4 / 3
 *  The smallest tokens it can make (no payload) then would be 39 characters
 *  assuming the cleartext nonce couldn't be compressed at all (raw token there
 *  is 29)
 *
 * Example:
 *  So if the plaintext was an uncompressed 4byte peer ip address + a 3 byte
 *  counter, we'd get: (1 + 12 + 4 + 3 + 16) * 4 / 3 = 48 bytes/characters
 *  long- so it might look something like this:
 *
 *  yZLD7Lp4E67xd_SCOq8x7SzxcpkMZ6vI0fLEA3XHJCgWEd7s
 *
 *
 *
 * Guaranteeing nonce uniqueness
 * ------------------------------
 * 
 * Above we're simply using the host IP address and ticks from the monotonic
 * clock. This leaves many possibilities for accidental nonce reuse, which
 * under most encryption schemes destroys the security. This explains some
 * possible future enhancements, if ever needed.
 *
 * Clock-ticks are vulnerable to accidental reuse when a virtual-machine rolls
 * back, when a clock is reset, when a system reboots or corrects its time,
 * etc.  The safer thing may be to create the nonce as follows (probably an
 * overkill which is why I'm just writing it down here in the comments while
 * it's on my mind instead of implementing it):
 *
 *  1. On process start
 *     - Guarantee CPU affinity for the process (?)
 *     - PART1 = clock_gettime(CLOCK_MONOTONIC_RAW, ...)
 *     - PART2 = some bytes pulled from /dev/random (_not_ /dev/urandom) - even
 *               if it blocks for a bit- it's process startup afterall.
 *     - PART3 = IPv4 IP address (or a MAC address, I suppose, if there's room)
 *     - COUNTR= start a monotonically increasing counter (with proper atomic
 *               operations).
 *     - (obviously if a process is the only thing that needs to later
 *       authenticate a token- create a new strong random key for just that
 *       process using /dev/random.- In general, narrow the scope of a key as
 *       much as possible)
 *
 *  2. When making a new nonce:
 *     - PART4 = current CPU-id for the process (cached if cpu-affinity locked
 *               in at process startup)
 *     - If possible using native atomic operations or possibly cross-process
 *       mutex/semaphores, increment-read COUNTR by 13 or occasionally by a
 *       value pulled from /dev/random times 17 (or some other prime number-
 *       see below for explanation- but ideally a _different_ prime number than
 *       the 13 or whatever is being used as the gap).
 *     - nonce = combine(PART1, PART2, PART3, PART4, COUNTR) (possibly some
 *       components could be combined via xor if concatenating makes this nonce
 *       just way too big. It would be a tradeoff because then you start to
 *       reintroduce collision risk. You could tune it so it specifically xors
 *       parts that add the least "uniqueness" to the nonce and can therefore)
 *
 * In this scheme:
 *     - PART3 protects against accidental repeat by different machines that
 *       are synced together in time and using the same key etc. (it can
 *       be improved, but combined with the other stuff it would be highly 
 *     - PART1 protects against accidental repeat due to rolling back the
 *       clock or due to processes on different CPUs getting the same time.
 *     - PART2 protects against accidental repeat due to virtual-machine
 *       rollback (macro-level, restore-point type rollbacks).
 *     - PART4, of course, guarantees uniqueness of nonces generated by
 *       processes on different CPUs.
 *     - COUNTR occasional /dev/random incrementing behavior protects against
 *       micro-level virtual-machine rollbacks. Don't know if they exist, but
 *       if not now, they may in the future.
 *     - Putting prime-number gaps on each COUNTR increment further protects
 *       against missuse in the following manner: If all else fails and two or
 *       more processes are generating nonce's only distinguished by COUNTR, a
 *       bigger prime-number gap will decrease the likelihood that a process
 *       will reuse a COUNTR value from another process. If the COUNTR in that
 *       situation is incrementing by one, nonce reuse is guaranteed- but with
 *       a gap we increase the probability that they'll keep leapfrogging each
 *       other.
 *
 *
 * Of course, even more robustness against accidental nonce-missuse could be
 * had by using AES-SIV, where even if a nonce _was_ accidentally reused, it
 * would only leak information if the attacker had plaintexts for the tokens
 * that had the repeated nonce.
 *
 */

#include <gx/gx.h>
//#include "ae.h"



/**
 * GUID:
 *  initialize once:
 *    - gx_node_uid   --> unique per network configuration (mac addrs / ip addrs)
 *    - gx_dev_random --> some intrinsic randomness for vm rollbacks
 *    - pid
 *    - thread-id
 *    - cpu-timestamp
 *
 */


/*static int gx_initialize_nonce() {

    return 0;
}*/



/// This will give high quality non-deterministic random data.
/// BUT it may block a bit on Linux, so use it only where needed or set
/// is_strict to false.
static int _gx_devrandom_fd  = -1;
static int _gx_devurandom_fd = -1;
static int gx_dev_random(void *dest, size_t len, int is_strict) {
    int     tries = 0;
    ssize_t rcv_count;
    if(gx_unlikely(len == 0 || dest == NULL)) {
        errno = EINVAL;
        return -1;
    }
init:
    if(gx_unlikely(_gx_devrandom_fd == -1))
        Xs( _gx_devrandom_fd = open("/dev/random", O_RDONLY | O_NONBLOCK)) {
            case EINTR: goto init;
            default:    X_RAISE(-1);
        }
    if(gx_unlikely(is_strict && (_gx_devurandom_fd == -1)))
        Xs( _gx_devurandom_fd = open("/dev/urandom", O_RDONLY)) {
            case EINTR: goto init;
            default:    X_RAISE(-1);
        }
exec:
    Xs(rcv_count = read(_gx_devrandom_fd, dest, len)) {
        case EINTR:   goto exec;
        case EAGAIN:  if(is_strict) {
                          if(tries++ < 4) {
                              gx_sleep(0,100);
                              goto exec;
                          } else {errno = EAGAIN; return -1;}
                      } else {
                          Xs(rcv_count = read(_gx_devurandom_fd, dest, len)) {
                              case EINTR: goto exec;
                              default:    close(_gx_devurandom_fd);
                                          _gx_devurandom_fd = -1;
                                          if(tries++ < 2) goto init;
                                          else X_RAISE(-1);
                          }
                      }
                      break;
        default:      close(_gx_devrandom_fd);
                      _gx_devrandom_fd = -1;
                      if(tries++ < 2) goto init;
                      else X_RAISE(-1);
    }
    if(rcv_count < len) {
        if(tries++ < 2) goto exec;
        else {errno = EIO; return -1;}
    }
    return 0;
}

#if __INTEL_COMPILER
  #define GX_CPUSTAMP ((unsigned)__rdtsc())
#elif (__GNUC__ && (__x86_64__ || __amd64__ || __i386__))
  #define GX_CPUSTAMP ({unsigned res; __asm__ __volatile__ ("rdtsc" : "=a"(res) : : "edx"); res;})
#elif (_M_IX86)
  #include <intrin.h>
  #pragma intrinsic(__rdtsc)
  #define GX_CPUSTAMP ((unsigned)__rdtsc())
#else
  #error Architechture not supported! No native construct for rdtsc
#endif


/**
 * gx_base64_urlencode_m3()
 *
 * Encodes inp into outp, optimized for inputs that are multiples of 3 in
 * length. Be sure that outp is allocated to be at least
 * GX_BASE64_SIZE(sizeof(input_data));
 *
 */
static const GX_OPTIONAL char _gx_t64[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
#define GX_BASE64_SIZE(DATSIZE) (4 * (DATSIZE) / 3 + 1)
static GX_INLINE ssize_t gx_base64_urlencode_m3(const void *indata, size_t insize, char *outdata) {
    const char *inp  = (const char *)indata;
    char       *outp = outdata;
    if(gx_unlikely(insize % 3 != 0)) {errno = EINVAL; return -1;}
    while(inp < (const char *)(indata + insize)) {
        outp[0] = _gx_t64[((inp[0] & 0xFC) >> 2) ];
        outp[1] = _gx_t64[((inp[0] & 0x03) << 4) | ((inp[1] & 0xF0) >> 4)];
        outp[2] = _gx_t64[((inp[1] & 0x0F) << 2) | ((inp[2] & 0xC0) >> 6)];
        outp[3] = _gx_t64[ (inp[2] & 0x3F)       ];
        inp += 3;
        outp += 4;
    }
    outp[0] = '\0';
    return outp + 1 - outdata;
}
