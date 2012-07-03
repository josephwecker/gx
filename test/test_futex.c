#include <gx/gx.h>
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
#include <gx/gx_mfd.h>
#include <sys/mman.h>


gx_error_initialize(GX_DEBUG);
#define ERR {X_FATAL; X_EXIT;}
int main(int argc, char **argv) {
    char    path[] = "tmp/rb-XXXXXX";
    int     fd;
    void   *fut_write, *fut_read;
    pid_t   parent;

    X (fd = mkstemp(path)) ERR;
    X (unlink(path)      ) ERR;
    X (ftruncate(fd, 32) ) ERR;

    X (parent = fork()) ERR;
    if(parent) {
        int wake_res;
        Xm(fut_write = mmap(NULL, gx_pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) ERR;
        X (mlock(fut_write, gx_pagesize)) ERR;
        sleep(1);
        printf("Parent: Changing data from 0 to 2\n");
        ((int *)fut_write)[0] = 2;
        sleep(1);
        printf("Parent: Calling gx_futex_wake\n");
        X (wake_res = gx_futex_wake(fut_write)) ERR;
        printf("Parent: Done, got %d from wake\n", wake_res);
    } else { // Child
        int res;
        Xm(fut_read  = mmap(NULL, gx_pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) ERR;
        X (mlock(fut_read,  gx_pagesize)) ERR;
        X (res = gx_futex_wait(fut_read, 0)) ERR;
        printf("Child: Done waiting- got %d\n", res);
    }

    return 0;
}
