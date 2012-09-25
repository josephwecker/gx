/**
 * Creates tokens that can easily be used in HTTP etc using some form of
 * authentication encryption. That is, the tokens confidentially convey data
 * and can be completely authenticated.
 *
 * Ideally it should cache everything it can. Possibly even cache the
 * clock-tick and reuse by all threads/processes.
 *
 * @todo  gx_nonce_next_base64 combo function to skip all those intermediates.
 * @todo  fix the node_uid so that it's binary to speed up initialization.
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
 *///--------------------------------------------------------------------------
#ifndef GX_TOKEN_H
#define GX_TOKEN_H

#include "./gx.h"
#include "./gx_net.h"
//#include "ae.h"

//-----------------------------------------------------------------------------
/// Will have to fetch more random data after this many nonces have been generated.
#define _GX_RPSIZE 256


//-----------------------------------------------------------------------------
/// Helpers for getting structure sizes
#define GX_BASE64_SIZE(DATSIZE) (4 * (DATSIZE) / 3 + 1)
#define GX_NONCE_BINSIZE 12
#define GX_NONCE_STRSIZE GX_BASE64_SIZE(GX_NONCE_BINSIZE)

//-----------------------------------------------------------------------------
typedef struct _gx_nm_identcomps {
    uint32_t          rand1;                    ///< Entropic random data as part of the signature
    char              node_uid[GX_NODE_UID_LEN];///< Unique per network config (ip-addrs + hdwr-addrs)
    uint64_t          ts1;                      ///< CPU timestamp when first allocated
    int               tid;                      ///< Result of gettid - like pid but unique when threaded

} _gx_nm_identcomps;

typedef union __attribute__((__packed__)) _gx_nonce {
    // Perspective as final nonce:
    volatile char         as_bytes[12];         ///< 12-byte nonce final value

    // Perspective when intializing:
    struct __attribute__((__packed__)) {
        uint64_t          ident_hash;           ///< 64-bit compression of the above fields
        uint8_t           rand2[4];             ///< Essentially the counter's starting position
    };

    // Perspective when atomically incrementing:
    struct __attribute__((__packed__)) {
        uint32_t          top_part;             ///< First 4 bytes of nonce don't change
        volatile uint64_t counter;              ///< Counter that overlaps w/ ident-hash
    };
} _gx_nonce;

typedef struct gx_nonce_machine {
    _gx_nm_identcomps     ident;                ///< Help us to be pretty darn unique
    volatile _gx_nonce    nonce;                ///< Actual current value
    uint8_t               rand_pool[_GX_RPSIZE];///< Used for increment values
    int                   rand_pool_pos;        ///< Points to current randpool pos
} gx_nonce_machine;


static int _gx_misc_primes[] GX_OPTIONAL = {11, 13, 17, 19, 23, 29, 31, 37, 41,
    43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197,
    199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
    283, 293, 307, 311, 313, 317, 331, 313};

//-----------------------------------------------------------------------------
/// Forward Declarations
static           int      gx_nonce_init(gx_nonce_machine *nm, int hardened);
static GX_INLINE int      gx_nonce_next(gx_nonce_machine *nm, char *buf);

static           int      gx_dev_random(void *dest, size_t len, int is_strict);
static GX_INLINE ssize_t  gx_base64_urlencode_m3(const void *indata, size_t insize, char *outdata);
static GX_INLINE uint64_t gx_hash64(const char *key, uint64_t len, uint64_t seed);


//-----------------------------------------------------------------------------
static int gx_nonce_init(gx_nonce_machine *nm, int hardened) {
    memset(nm, 0, sizeof(*nm));
    X (gx_node_uid  (nm->ident.node_uid)                                    ) X_RAISE(-1);
    X (gx_dev_random(&(nm->ident.rand1), sizeof(nm->ident.rand1), hardened) ) X_RAISE(-1);
    X (gx_dev_random(nm->rand_pool,      sizeof(nm->rand_pool),   0)        ) X_RAISE(-1);
    // The following are direct syscalls so that glibc etc. doesn't cache the values.
  #ifdef _LINUX
    X (nm->ident.tid     = syscall(SYS_gettid)) X_RAISE(-1);
  #else
    X (nm->ident.tid     = syscall(SYS_thread_selfid)) X_RAISE(-1);
  #endif
    nm->ident.ts1        = GX_CPU_TS;
    nm->nonce.rand2[0]   = nm->rand_pool[0];
    nm->nonce.rand2[1]   = nm->rand_pool[1];
    nm->nonce.rand2[2]   = nm->rand_pool[2];
    nm->nonce.rand2[3]   = nm->rand_pool[3];
    nm->nonce.ident_hash = gx_hash64((void *)&(nm->ident), sizeof(nm->ident), (uint64_t)nm->rand_pool[4]);
    nm->rand_pool_pos    = 5;
    return 0;
}


//-----------------------------------------------------------------------------
/// Defaulting to 12 byte nonces at the moment.
static GX_INLINE int gx_nonce_next(gx_nonce_machine *nm, char *buf) {
    _gx_nonce res;

    // Double checking for tid changes doesn't seem to add much overhead at
    // all, so keeping it here for now to avoid collisions when the same
    // gx_nonce_machine instance is initialized _before_ forking /
    // thread-spawning. Feel free to remove this part for a small speed boost
    // if you're absolutely certain that the nonce-machine is initialized
    // _after_ any forking/cloning/spawning.
    int new_tid;
  #ifdef _LINUX
    X (new_tid = syscall(SYS_gettid)) X_RAISE(-1);
  #else
    X (new_tid = syscall(SYS_thread_selfid)) X_RAISE(-1);
  #endif
    if(gx_unlikely(new_tid != nm->ident.tid)) {
        nm->ident.tid = new_tid;
        nm->nonce.ident_hash = gx_hash64((void *)&(nm->ident), sizeof(nm->ident), (uint64_t)nm->rand_pool[4]);
    }

    if(gx_unlikely(nm->rand_pool_pos >= _GX_RPSIZE)) {
        X (gx_dev_random(nm->rand_pool, sizeof(nm->rand_pool), 0)) X_RAISE(-1);
        nm->rand_pool_pos = 0;
    }

    uint8_t      rnd   = nm->rand_pool[nm->rand_pool_pos++];
    unsigned int incby = _gx_misc_primes[rnd & 0x3f] * (((rnd & 0xc0) >> 6) + 1);

    res.top_part = nm->nonce.top_part;
    res.counter  = __sync_add_and_fetch(&nm->nonce.counter, incby);

    memcpy(buf, (char *)res.as_bytes, sizeof(res.as_bytes));
    return 0;
}



//-----------------------------------------------------------------------------
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
    if(gx_unlikely(!is_strict && (_gx_devurandom_fd == -1)))
        Xs( _gx_devurandom_fd = open("/dev/urandom", O_RDONLY)) {
            case EINTR: goto init;
            default:    X_RAISE(-1);
        }
exec:
    Xs(rcv_count = read(_gx_devrandom_fd, dest, len)) {
        case EINTR:   goto exec;
        case EAGAIN:  if(is_strict) {
                          if(tries++ < 10) {
                              gx_sleep(2,100);
                              goto exec;
                          } else {errno = EAGAIN; return -1;}
                      } else {
                          X_LOG_WARN("Getting subpar random numbers.");
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
    if((size_t)rcv_count < len) {
        if(tries++ < 2) goto exec;
        else {errno = EIO; return -1;}
    }
    return 0;
}

//-----------------------------------------------------------------------------
/**
 * gx_base64_urlencode_m3()
 *
 * Encodes inp into outp, optimized for inputs that are multiples of 3 in
 * length. Be sure that outp is allocated to be at least
 * GX_BASE64_SIZE(sizeof(input_data));
 *
 */
//static const GX_OPTIONAL char _gx_t64[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static const GX_OPTIONAL char _gx_t64[]= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz-";
#define GX_BASE64_SIZE(DATSIZE) (4 * (DATSIZE) / 3 + 1)
//static GX_INLINE ssize_t gx_base64_urlencode_m3(const void *indata, size_t insize, char *outdata) {
static GX_INLINE ssize_t gx_base64_urlencode_m3(const void *indata, size_t insize, char *outdata) {
    const char *inp  = (const char *)indata;
    char       *outp = outdata;
    if(gx_unlikely(insize % 3 != 0)) {errno = EINVAL; return -1;}
    while(inp < (const char *)(indata + insize)) {
        outp[0] = _gx_t64[((inp[0] & 0xFC) >> 2) ];
        outp[1] = _gx_t64[((inp[0] & 0x03) << 4) | ((inp[1] & 0xF0) >> 4)];
        outp[2] = _gx_t64[((inp[1] & 0x0F) << 2) | ((inp[2] & 0xC0) >> 6)];
        outp[3] = _gx_t64[ (inp[2] & 0x3F)       ];
        inp    += 3;
        outp   += 4;
    }
    outp[0] = '\0';
    return outp + 1 - outdata;
}


//-----------------------------------------------------------------------------
/// Very fast 64 bit hash of misc data with good properties. Only works on 64-bit machines.
/// Based on CrapWow64 - http://www.team5150.com/~andrew/noncryptohashzoo/CrapWow64.html
/// Unknown license. It has close to ideal lack of collisions and speed.
static GX_INLINE uint64_t gx_hash64(const char *key, uint64_t len, uint64_t seed) {
    const uint64_t m = 0x95b47aa3355ba1a1, n = 0x8a970be7488fda55;
    uint64_t hash;
    // 3 = m, 4 = n
    // r12 = h, r13 = k, ecx = seed, r12 = key
    asm(
        "leaq (%%rcx,%4), %%r13\n"
        "movq %%rdx, %%r14\n"
        "movq %%rcx, %%r15\n"
        "movq %%rcx, %%r12\n"
        "addq %%rax, %%r13\n"
        "andq $0xfffffffffffffff0, %%rcx\n"
        "jz QW%=\n"
        "addq %%rcx, %%r14\n\n"
        "negq %%rcx\n"
    "XW%=:\n"
        "movq %4, %%rax\n"
        "mulq (%%r14,%%rcx)\n"
        "xorq %%rax, %%r12\n"
        "xorq %%rdx, %%r13\n"
        "movq %3, %%rax\n"
        "mulq 8(%%r14,%%rcx)\n"
        "xorq %%rdx, %%r12\n"
        "xorq %%rax, %%r13\n"
        "addq $16, %%rcx\n"
        "jnz XW%=\n"
    "QW%=:\n"
        "movq %%r15, %%rcx\n"
        "andq $8, %%r15\n"
        "jz B%=\n"
        "movq %4, %%rax\n"
        "mulq (%%r14)\n"
        "addq $8, %%r14\n"
        "xorq %%rax, %%r12\n"
        "xorq %%rdx, %%r13\n"
    "B%=:\n"
        "andq $7, %%rcx\n"
        "jz F%=\n"
        "movq $1, %%rdx\n"
        "shlq $3, %%rcx\n"
        "movq %3, %%rax\n"
        "shlq %%cl, %%rdx\n"
        "addq $-1, %%rdx\n"
        "andq (%%r14), %%rdx\n"
        "mulq %%rdx\n"
        "xorq %%rdx, %%r12\n"
        "xorq %%rax, %%r13\n"
    "F%=:\n"
        "leaq (%%r13,%4), %%rax\n"
        "xorq %%r12, %%rax\n"
        "mulq %4\n"
        "xorq %%rdx, %%rax\n"
        "xorq %%r12, %%rax\n"
        "xorq %%r13, %%rax\n"
        : "=a"(hash), "=c"(key), "=d"(key)
        : "r"(m), "r"(n), "a"(seed), "c"(len), "d"(key)
        : "%r12", "%r13", "%r14", "%r15", "cc");
    return hash;
/*#elif !defined(INTEL)
   #define cwfold(a, b, lo, hi) { p = (uint64_t)(a) * (u128)(b); lo ^= (uint64_t)p; hi ^= (uint64_t)(p >> 64); }
   #define cwmixa( in ) { cwfold( in, m, k, h ); }
   #define cwmixb( in ) { cwfold( in, n, h, k ); }

    const uint64_t *key8 = (const uint64_t *)key;
    uint64_t h = len, k = len + seed + n;
    u128 p;

    while ( len >= 16 ) { cwmixb(key8[0]) cwmixa(key8[1]) key8 += 2; len -= 16; }
    if ( len >= 8 ) { cwmixb(key8[0]) key8 += 1; len -= 8; }
    if ( len ) { cwmixa( key8[0] & ( ( (uint64_t)1 << ( len * 8 ) ) - 1 ) ) }
    cwmixb( h ^ (k + n) )
        return k ^ h;
#endif
*/
}
#endif
