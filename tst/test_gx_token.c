#include "../gx.h"
#include "../gx_net.h"
#include "../gx_token.h"

gx_error_initialize(GX_DEBUG);

int main(int argc, char **argv) {
    int r1, r2;
    uint64_t r3;
    char nuid[GX_NODE_UID_LEN] = "";

    _ (gx_dev_random(&r1, sizeof(r1), 1)) _error();
    _ (gx_dev_random(&r2, sizeof(r2), 1)) _error();
    _ (gx_dev_random(&r3, sizeof(r3), 0)) _error();

    log_info("Some true randomness: r1: %x | r2: %x | r3: %llx", r1, r2, r3);

    _ (gx_node_uid(nuid)) _error();
    log_info("Returned network UID:          %s", nuid);
    _ (gx_node_uid(nuid)) _error();
    log_info("Returned network UID (pass 2): %s", nuid);

    log_info("CPU timestamp: %llx", cpu_ts);
    log_info("CPU timestamp: %llx", cpu_ts);

    char     thedata[] = {0x12,0xff,0x31,0x12,0x90,0x05,0x74,0x12,0x75,0x19,0x02,0x12};
    #define  b64_len GX_BASE64_SIZE(sizeof(thedata))
    char     thedata_b64[b64_len];
    ssize_t  out_len;
    _ (out_len = gx_base64_urlencode_m3(thedata, sizeof(thedata), thedata_b64)) _error();
    log_info("Base64-encoded data (%llu/%llu): %s", out_len, b64_len, thedata_b64);

    log_debug("size: %llu", sizeof(_gx_misc_primes) / sizeof(int));


    char blahblah[] = "alahblah.";
    log_debug("hash of 'alahblah.': %llx", gx_hash64(blahblah, sizeof(blahblah), 0));

    gx_nonce_machine nm;
    _ (gx_nonce_init(&nm, 0)) _error();


    log_debug("\nrand1:      %llx\n"
                  "node_uid:   %s\n"
                  "ts1:        %llx\n"
                  "tid:        %u\n\n"
                  "ident_hash: %llx\n"
                  "rand2:      %02x|%02x|%02x|%02x\n",
                nm.ident.rand1,
                nm.ident.node_uid,
                nm.ident.ts1,
                nm.ident.tid,
                nm.nonce.ident_hash,
                nm.nonce.rand2[0], nm.nonce.rand2[1], nm.nonce.rand2[2], nm.nonce.rand2[3]);

    //gx_hexdump(&(nm.nonce), sizeof(nm.nonce), 0);

    char nonce[12];
    char nonce_base64[GX_BASE64_SIZE(12)];
    int i;
    for(i=0; i<10; i++) {
        _ (gx_nonce_next(&nm, nonce)) _error();
        _ (gx_base64_urlencode_m3(nonce, 12, nonce_base64)) _error();
        log_info("Nonce #%d: %s", i, nonce_base64);
    }

    return 0;
}
