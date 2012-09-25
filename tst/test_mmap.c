
/* to create the necessary testfile:
 *
 * $  dd if=/dev/urandom of=testrand.txt bs=4096 count=10
 *
 */

#include "../gx.h"
#include "../gx_error.h"
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>

gx_error_initialize(GX_DEBUG);

int main(int argc, char **argv) {
    int      fd;
    uint8_t *full_map;
    char     path[] = "tmp/testrand.txt";
    _(  fd=open(path,O_RDWR|O_NONBLOCK|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) _abort();
    //_(  fstat(fd, &filestat)                                                             ) _raise(-1);
    //curr_size = filestat.st_size;
    //mfd->map_size = max(curr_size, pagesize());
    _M( full_map = mmap(NULL, pagesize() << 1, PROT_READ|PROT_WRITE, MAP_SHARED, fd,0)) _abort();

    printf("%x | %x\n", full_map[0], full_map[pagesize()]);

#ifdef __LINUX__
    _M( full_map = mremap(full_map, pagesize() << 1, pagesize() * 3, MREMAP_MAYMOVE)) _abort();
#else
    #error mremap not yet implemented on mac (TODO)
#endif

    printf("%x | %x | %x\n", full_map[0], full_map[pagesize()], full_map[pagesize() << 1]);

    return 0;
}
