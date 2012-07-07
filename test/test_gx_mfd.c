#include <gx/gx.h>
#include <gx/gx_mfd.h>
#include <gx/gx_event.h>

gx_error_initialize(GX_DEBUG);


gx_mfd_pool *mfd_pool;

void writer() {
    int     i;
    gx_mfd *mfd_w;
    char    msg[] = "something blah blah";

    Xn( mfd_w = acquire_gx_mfd(mfd_pool)               ){X_ERROR; X_EXIT;}
    X ( gx_mfd_create_w(mfd_w,10,"tmp/writer-file.dat")){X_ERROR; X_EXIT;}
    gx_sleep(1,0);
    for(i=0; i < 5; i++) {
        gx_sleep(0,500000000);
        X_LOG_INFO("Writing some stuff");
        mfd_write(mfd_w, msg, sizeof(msg));
    }
    //printf("writer: %llX\n", (unsigned long long int)(mfd_w->head->sig));
    gx_sleep(1,0);
}

void reader() {
    gx_mfd                 *mfd_r;
    //struct GX_EVENT_STRUCT  events[5];
    //int                     evfd = gx_event_newset(5);
    Xn( mfd_r = acquire_gx_mfd(mfd_pool)               ){X_ERROR; X_EXIT;}
    X ( gx_mfd_create_r(mfd_r,10,"tmp/writer-file.dat")){X_ERROR; X_EXIT;}
    gx_sleep(2,0);
    printf("reader: %llX\n", (unsigned long long int)(mfd_r->head->sig));
    gx_sleep(3,0);
}


int main(int argc, char **argv) {
    pid_t pid;
    int   status;

    Xn( mfd_pool = new_gx_mfd_pool(2) ) {X_ERROR; X_EXIT;}
    X (pid = fork()                   ) {X_ERROR; X_EXIT;}
    if(pid) /* child  */ writer();
    else    /* parent */ {reader(); waitpid(pid, &status, 0);}
    return status;
}
