#include "../gx.h"
#include "../ext/sha2.h"

gx_error_initialize(GX_DEBUG);

int main(int argc, char **argv) {
    char hashed[128];
    hashed[0] = '\0';
    SHA256_Data((sha2_byte *)"The quick brown fox jumps over the lazy dog", (size_t)43, hashed);
    hashed[127] = '\0';

    log_info("The hash seems to be: %s", hashed);
    return 0;
}
