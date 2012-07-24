#include <gx/gx.h>
#include <gx/gx_net.h>
#include <gx/gx_token.h>

gx_error_initialize(GX_DEBUG);

int main(int argc, char **argv) {
    int r1, r2;
    uint64_t r3;
    char nuid[GX_NODE_UID_LEN] = "";

    X (gx_dev_random(&r1, sizeof(r1))) X_ERROR;
    X (gx_dev_random(&r2, sizeof(r2))) X_ERROR;
    X (gx_dev_random(&r3, sizeof(r3))) X_ERROR;

    X_LOG_INFO("Some true randomness: r1: %x | r2: %x | r3: %llx", r1, r2, r3);

    X (gx_node_uid(nuid)) X_ERROR;
    X_LOG_INFO("Returned network UID:          %s", nuid);
    X (gx_node_uid(nuid)) X_ERROR;
    X_LOG_INFO("Returned network UID (pass 2): %s", nuid);


    return 0;
}
