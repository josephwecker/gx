#include "../gx.h"
/*#ifdef __LINUX__
  #ifndef _GNU_SOURCE
    #define _GNU_SOURCE
  #endif
  #include <unistd.h>
  #include <sys/syscall.h>
  #include <sys/types.h>
  #include <linux/futex.h>
  //#include <sys/time.h>
#endif
*/
#include "../gx_mfd.h"
#include <sys/mman.h>


gx_error_initialize(GX_DEBUG);
#define ERR _abort();
int main(int argc, char **argv) {
    char    path[] = "tmp/rb-XXXXXX";
    int     fd;
    void   *fut_write, *fut_read;
    pid_t   parent;

    _ (fd = mkstemp(path)) ERR;
    _ (unlink(path)      ) ERR;
    _ (ftruncate(fd, 32) ) ERR;

    _ (parent = fork()) ERR;
    if(parent) {
        int wake_res;
        _M(fut_write = mmap(NULL, pagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) ERR;
        _ (mlock(fut_write, pagesize())) ERR;
        sleep(1);
        printf("Parent: Changing data from 0 to 2\n");
        ((int *)fut_write)[0] = 2;
        sleep(1);
        printf("Parent: Calling gx_futex_wake\n");
        _ (wake_res = gx_futex_wake(fut_write)) ERR;
        printf("Parent: Done, got %d from wake\n", wake_res);
    } else { // Child
        int res;
        _M(fut_read  = mmap(NULL, pagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) ERR;
        _ (mlock(fut_read,  pagesize())) ERR;
        _ (res = gx_futex_wait(fut_read, 0)) ERR;
        printf("Child: Done waiting- got %d\n", res);
    }

    return 0;
}
