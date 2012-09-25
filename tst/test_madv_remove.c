/**
 *
 * This is an interesting one- mostly testing exactly what happens to memory
 * pages when various madvise constants are given- especially looking at the
 * behavior of MADV_REMOVE.
 *
 * @todo:   Look at behavior when second process opens the file, maps etc.
 *          after the first has already REMOVE'd blocks.
 * @todo:   Look at behavior when normally mounted files used instead of tmpfs
 *
 */


#include "../gx.h"
#include "../gx_error.h"
#include "../gx_system.h"
#include <sys/mman.h>

gx_error_initialize(GX_DEBUG);


#define TOTAL_PAGES    1000
#define FREE_PAGES      200
#define DONTNEED_PAGES  200


#ifdef __LINUX__
char    path[] = "/dev/shm/tmptest-XXXXXX";
#else
char    path[] = "/tmp/tmptest-XXXXXX";
#endif
int shfd;

int child() {
    gx_sys_refresh();
    X_LOG_INFO("[child]  RAM in use before mmap:                  %4llu", gx_sys_ram_pages_in_use());
    uint8_t *map, *p;
    size_t len;
    Xm(map = mmap((void *)NULL, pagesize() * TOTAL_PAGES, PROT_READ | PROT_WRITE, MAP_SHARED, shfd, 0)) {X_ERROR; X_RAISE(-1);}
    X (close(shfd))  X_WARN;
    gx_sys_refresh();
    X_LOG_INFO("[child]  RAM in use after mmap, before writing:   %4llu", gx_sys_ram_pages_in_use());

    // 1. write a byte to each page, report RAM usage
    int i;
    for(i=0; i<TOTAL_PAGES; i++) {
        p = map + (i * pagesize());
        X (madvise(p, pagesize(), MADV_WILLNEED)) X_WARN;
        *p = 0xAB;
    }
    gx_sys_refresh();
    X_LOG_INFO("[child]  RAM in use after %d pages written to:  %4llu", TOTAL_PAGES, gx_sys_ram_pages_in_use());

    // 2. (sleep)
    gx_sleep(1,0);

    // 3. mark first 2 pages as MADV_REMOVE, and page 3 as MADV_DONTNEED
    unsigned char testvec[3];

    gx_sys_refresh();
    X_LOG_INFO("[child]  RAM in use after sleeping:               %4llu", gx_sys_ram_pages_in_use());
    p = map; len = FREE_PAGES*pagesize();
    X (madvise(p, len, MADV_REMOVE)) X_WARN;
    X (mincore(map, pagesize() * 3, testvec)) X_ERROR;
    X_LOG_INFO("[child]  Residency test for removed:         %x/%x/%x/...", testvec[0], testvec[1], testvec[2]);
    X (munmap (p, len)             ) X_WARN;
    gx_sys_refresh();
    X_LOG_INFO("[child]  RAM in use after remove+unmap:           %4llu", gx_sys_ram_pages_in_use());
    p = map + (FREE_PAGES * pagesize()); len = DONTNEED_PAGES*pagesize();
    X (madvise(p, len, MADV_DONTNEED)) X_WARN;
    X (mincore(map + ((FREE_PAGES+1)*pagesize()), pagesize() * 3, testvec)) X_ERROR;
    X_LOG_INFO("[child]  Residency test for dontneeds:       %x/%x/%x/...", testvec[0], testvec[1], testvec[2]);
    X (munmap (p, len)               ) X_WARN;
    gx_sys_refresh();
    X_LOG_INFO("[child]  RAM in use after dontneed+unmap:         %4llu", gx_sys_ram_pages_in_use());


    // 4. (sleep)
    gx_sleep(1,0);
    gx_sys_refresh();
    X_LOG_INFO("[child]  Final RAM check after parent's done:     %4llu", gx_sys_ram_pages_in_use());

    X (unlink(path)) X_IGNORE;
    return 0;
}

int parent() {
    uint8_t *map, *p;
    Xm(map = mmap((void *)NULL, pagesize() * TOTAL_PAGES, PROT_READ | PROT_WRITE, MAP_SHARED, shfd, 0)) {X_ERROR; X_RAISE(-1);}
    X (close(shfd))  X_WARN;

    // 1. (sleep)
    gx_sleep(0,5000000);

    // 2. read the byte on each page. report RAM usage again
    gx_sys_refresh();
    X_LOG_INFO("[parent]   RAM in use after mapped but not read:  %4llu", gx_sys_ram_pages_in_use());
    int i;
    for(i=0; i<TOTAL_PAGES; i++) {
        p = map + (i * pagesize());
        if(*p != 0xAB) {
            X_LOG_ERROR("Shared mapping didn't see what the other process wrote!");
            return 10;
        }
    }
    X_LOG_INFO("[parent]   Correctly saw the stuff child wrote        ");
    gx_sys_refresh();
    X_LOG_INFO("[parent]   RAM in use after reading mapped pgs:   %4llu", gx_sys_ram_pages_in_use());

    // 3. (sleep)
    gx_sleep(1,0);

    // 4. assert correct byte still in last two pages,
    //    then assert correct byte still in DONTNEED pages,
    //    then assert that the removed pages are cleared more or less.

    gx_sys_refresh();
    X_LOG_INFO("[parent]   RAM in use after child's carnage:      %4llu", gx_sys_ram_pages_in_use());
    for(i=FREE_PAGES+DONTNEED_PAGES; i<TOTAL_PAGES; i++) {
        p = map + (i * pagesize());
        if(*p != 0xAB){X_LOG_ERROR("Shared mapping didn't see what the other process wrote!"); break;}
    }
    gx_sys_refresh();
    X_LOG_INFO("[parent]   RAM in use after inspecting actives:   %4llu", gx_sys_ram_pages_in_use());
    for(i=FREE_PAGES; i<FREE_PAGES+DONTNEED_PAGES; i++) {
        p = map + (i * pagesize());
        if(*p != 0xAB){X_LOG_ERROR("Shared mapping didn't see what the other process wrote!"); break;}
    }
    gx_sys_refresh();
    X_LOG_INFO("[parent]   RAM in use after inspecting dontneeds: %4llu", gx_sys_ram_pages_in_use());

    unsigned char testvec[3];
    X (mincore(map, pagesize() * 3, testvec)) X_ERROR;
    X_LOG_INFO("[parent]   Residency test for removed:       %x/%x/%x/...", testvec[0], testvec[1], testvec[2]);
    X (mincore(map + ((FREE_PAGES+DONTNEED_PAGES)*pagesize()), pagesize() * 3, testvec)) X_ERROR;
    X_LOG_INFO("[parent]   Residency test for actives:       %x/%x/%x/...", testvec[0], testvec[1], testvec[2]);

    for(i=0; i<FREE_PAGES; i++) {
        p = map + (i * pagesize());
        if(*p == 0xAB){X_LOG_ERROR("Expecting to _not_ see something here... was removed...(%d)",i); break;}
    }
    gx_sys_refresh();
    X_LOG_INFO("[parent]   RAM in use after inspecting removed:   %4llu", gx_sys_ram_pages_in_use());

    X (unlink(path)) X_IGNORE;
    return 0;
}


int main(int argc, char **argv) {
    pid_t pid = 0;
    int status1, status2;
    X (shfd = mkostemp(path, O_NOATIME)          ) {X_ERROR; X_EXIT;}
    X (ftruncate(shfd, pagesize() * TOTAL_PAGES)) {X_ERROR; X_EXIT;}
    X (pid  = fork()                   ) {X_ERROR; X_EXIT;}
    if(!pid) return child();
    else    {status1 = parent(); waitpid(pid, &status2, 0);}
    return status1 | status2;
}
