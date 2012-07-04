#include <gx/gx.h>
//#ifndef _BSD_SOURCE
//#define _BSD_SOURCE
//#endif
//#include <unistd.h>
#include <sched.h>
gx_error_initialize(GX_DEBUG);


int child(void *n) {

    //unsigned int usecs = drand48() * 100000000;
    //char msg[20];
    //snprintf(msg, 19, "/%x", usecs/10000);
    //usleep(usecs);
    //write(STDOUT_FILENO, msg, sizeof(msg));
    //write(STDOUT_FILENO, msg, 0);
    //sched_yield();
    return 0;
}


int main(int argc, char **argv) {
    int i;

    for(i=0; i < 100000; i++) {
        char msg[] = "-";
        write(STDOUT_FILENO, msg, sizeof(msg));
        X(gx_clone(&child, NULL)) {X_FATAL; X_EXIT;}
    }
    printf("\n (all-spawned) \n");
    //sleep(4);
    return 0;
}
