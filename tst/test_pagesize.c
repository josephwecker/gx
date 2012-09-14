#include <gx/gx.h>
gx_error_initialize(GX_DEBUG);

int main(int argc, char **argv) {
    int page_size = gx_pagesize;
    printf("0x%X\n0x%X\n", page_size, gx_pagesize);
    return 0;
}
