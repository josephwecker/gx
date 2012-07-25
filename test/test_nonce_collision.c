#include <gx/gx.h>
#include <gx/gx_net.h>
#include <gx/gx_token.h>

gx_error_initialize(GX_ERROR);

char nonce[12];
char nonce_base64[GX_BASE64_SIZE(12)];
gx_nonce_machine nm;

int main(int argc, char **argv) {
    fork();
    X (gx_nonce_init(&nm, 0)) X_ERROR;
    fork();
    int i;
    for(i=0; i<1000000; i++) {
        X (gx_nonce_next(&nm, nonce)) {X_FATAL; X_EXIT;}
        X (gx_base64_urlencode_m3(nonce, 12, nonce_base64)) {X_FATAL; X_EXIT;}
        flockfile(stdout);
        nonce_base64[GX_BASE64_SIZE(12) - 1] = '\n';
        X(write(STDOUT_FILENO, nonce_base64, sizeof(nonce_base64))) X_ERROR;
        funlockfile(stdout);
    }

    return 0;
}

