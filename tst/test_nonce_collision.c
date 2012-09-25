#include "../gx.h"
#include "../gx_net.h"
#include "../gx_token.h"

gx_error_initialize(GX_ERROR);

char nonce[12];
char nonce_base64[GX_BASE64_SIZE(12)];
gx_nonce_machine nm;

int main(int argc, char **argv) {
    fork();
    _ (gx_nonce_init(&nm, 0)) _error();
    fork();
    int i;
    for(i=0; i<100000; i++) {
        _ (gx_nonce_next(&nm, nonce)) _abort();
        _ (gx_base64_urlencode_m3(nonce, 12, nonce_base64)) _abort();
        flockfile(stdout);
        nonce_base64[GX_BASE64_SIZE(12) - 1] = '\n';
        _(write(STDOUT_FILENO, nonce_base64, sizeof(nonce_base64))) _error();
        funlockfile(stdout);
    }

    return 0;
}

