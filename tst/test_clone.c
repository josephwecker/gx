#include <gx/gx.h>
//#ifndef _BSD_SOURCE
//#define _BSD_SOURCE
//#endif
//#include <unistd.h>
#include <sched.h>
gx_error_initialize(GX_DEBUG);


int child(void *n) {
    //volatile unsigned int usecs = drand48() * 2000000;
    //char msg[20];
    //snprintf(msg, 19, "/%x", usecs/10000);
    return 0;
}


int main(int argc, char **argv) {
    int i;

    X_LOG_INFO("Spawning 20,000 shortlived light threads");
    for(i=0; i < 20000; i++) {
        X(gx_clone(&child, NULL)) {X_FATAL; X_EXIT;}
    }
    X_LOG_INFO("(all-spawned - waiting a couple of seconds as they continue to die)");

    gx_sleep(2,0);
    return 0;
}
