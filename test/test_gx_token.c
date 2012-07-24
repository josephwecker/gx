#include <gx/gx.h>
#include <gx/gx_net.h>
#include <gx/gx_token.h>
#include <pthread.h>

gx_error_initialize(GX_DEBUG);

int main(int argc, char **argv) {
    int r1, r2;
    uint64_t r3;
    char nuid[GX_NODE_UID_LEN] = "";

    X (gx_dev_random(&r1, sizeof(r1), 1)) X_ERROR;
    X (gx_dev_random(&r2, sizeof(r2), 1)) X_ERROR;
    X (gx_dev_random(&r3, sizeof(r3), 0)) X_ERROR;

    X_LOG_INFO("Some true randomness: r1: %x | r2: %x | r3: %llx", r1, r2, r3);

    X (gx_node_uid(nuid)) X_ERROR;
    X_LOG_INFO("Returned network UID:          %s", nuid);
    X (gx_node_uid(nuid)) X_ERROR;
    X_LOG_INFO("Returned network UID (pass 2): %s", nuid);

    X_LOG_INFO("CPU timestamp: %llx", GX_CPU_TS);
    X_LOG_INFO("CPU timestamp: %llx", GX_CPU_TS);

    char     thedata[] = {0x12,0xff,0x31,0x12,0x90,0x05,0x74,0x12,0x75,0x19,0x02,0x12};
    #define  b64_len GX_BASE64_SIZE(sizeof(thedata))
    char     thedata_b64[b64_len];
    ssize_t  out_len;
    X (out_len = gx_base64_urlencode_m3(thedata, sizeof(thedata), thedata_b64)) X_ERROR;
    X_LOG_INFO("Base64-encoded data (%llu/%llu): %s", out_len, b64_len, thedata_b64);

    X_LOG_DEBUG("size: %llu", sizeof(_gx_misc_primes) / sizeof(int));


    char blahblah[] = "alahblah.";
    X_LOG_DEBUG("hash of 'alahblah.': %llx", gx_hash64(blahblah, sizeof(blahblah), 0));



    return 0;
}
