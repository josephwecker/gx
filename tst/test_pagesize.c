#include "../gx.h"
gx_error_initialize(GX_DEBUG);

int main(int argc, char **argv) {
    int page_size = pagesize();
    printf("0x%_\n0x%_\n", page_size, pagesize());
    return 0;
}
