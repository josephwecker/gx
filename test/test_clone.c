#include <gx/gx.h>
//#ifndef _BSD_SOURCE
//#define _BSD_SOURCE
//#endif
//#include <unistd.h>
#include <sched.h>
gx_error_initialize(GX_DEBUG);


int child(void *n) {

    volatile unsigned int usecs = drand48() * 2000000;
    char msg[20];
    snprintf(msg, 19, "/%x", usecs/10000);
    //usleep(10000);
    //char msg[] = ".";
    //write(STDOUT_FILENO, msg, sizeof(msg));
    //write(STDOUT_FILENO, msg, 0);
    //sched_yield();
    return 0;
}


int main(int argc, char **argv) {
    int i, s;
    struct timespec req, rem;

    for(i=0; i < 20000; i++) {
        X(gx_clone(&child, NULL)) {X_FATAL; X_EXIT;}
    }
    char msg[] = "\n (all-spawned - waiting 3s) \n";
    write(STDOUT_FILENO, msg, sizeof(msg));

    req.tv_sec = 3;
    req.tv_nsec = 0;
    for(;;) {
        Xs(s = nanosleep(&req, &rem)) {
            case EINTR: req = rem;
                        break;
            default:    X_ERROR; X_EXIT;
        }
        if(!s) break;
    }
    return 0;
}
