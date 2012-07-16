
/* to create the necessary testfile:
 *
 * $  dd if=/dev/urandom of=testrand.txt bs=4096 count=10
 *
 */

#include <gx/gx.h>
#include <gx/gx_error.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>

gx_error_initialize(GX_DEBUG);

int main(int argc, char **argv) {
    int      fd;
    uint8_t *full_map;
    char     path[] = "tmp/testrand.txt";
    X(  fd=open(path,O_RDWR|O_NONBLOCK|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) {X_FATAL; X_EXIT;}
    //X(  fstat(fd, &filestat)                                                             ) X_RAISE(-1);
    //curr_size = filestat.st_size;
    //mfd->map_size = MAX(curr_size, gx_pagesize);
    Xm( full_map = mmap(NULL, gx_pagesize << 1, PROT_READ|PROT_WRITE, MAP_SHARED, fd,0)) {X_FATAL; X_EXIT;}

    printf("%x | %x\n", full_map[0], full_map[gx_pagesize]);
    
#ifdef __LINUX__
    Xm( full_map = mremap(full_map, gx_pagesize << 1, gx_pagesize * 3, MREMAP_MAYMOVE)) {X_FATAL; X_EXIT;}
#else
    #error mremap not yet implemented on mac (TODO)
#endif

    printf("%x | %x | %x\n", full_map[0], full_map[gx_pagesize], full_map[gx_pagesize << 1]);

    return 0;
}
