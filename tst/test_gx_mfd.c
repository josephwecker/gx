#include "../gx.h"
#include "../gx_mfd.h"
#include "../gx_event.h"

gx_error_initialize(GX_DEBUG);


gx_mfd_pool *mfd_pool;

void writer() {
    int     i;
    gx_mfd *mfd_w;
    char    msg[] = "something blah blah";

    _N( mfd_w = acquire_gx_mfd(mfd_pool)               ) _abort();
    _ ( gx_mfd_create_w(mfd_w,10,"tmp/writer-file.dat")) _abort();
    gx_sleep(1);
    for(i=0; i < 5; i++) {
        gx_sleep(0,500);
        log_info("CHILD:    Writing some stuff");
        mfd_write(mfd_w, msg, sizeof(msg));
    }
    //printf("writer: %llX\n", (unsigned long long int)(mfd_w->head->sig));
    gx_sleep(1);
}

gx_eventloop_declare  (mfd_events, 1, 1);
gx_eventloop_implement(mfd_events, 1, 1);

int on_readfile_changed(gx_tcp_sess *s, uint32_t event) {
    log_info("PARENT:   Eventloop told me the file has new data.");
    return 0;
}

void reader() {
    gx_mfd                 *mfd_r;
    gx_eventloop_init (mfd_events);
    gx_sleep(0,500);  // Give the writer enough time to get the file set up.
    _N( mfd_r = acquire_gx_mfd(mfd_pool)               ) _abort();
    _ ( gx_mfd_create_r(mfd_r,10,"tmp/writer-file.dat")) _abort();
    _ ( mfd_events_add_misc(mfd_r->notify_fd, mfd_r)   ) _abort();
    _ ( mfd_events_wait(-1, on_readfile_changed)       ) _abort();
}


int main(int argc, char **argv) {
    pid_t pid;
    int   status=0;

    _N( mfd_pool = new_gx_mfd_pool(2) ) _abort();
    _ (pid = fork()                   ) _abort();
    if(pid) /* child  */ writer();
    else    /* parent */ {reader(); waitpid(pid, &status, 0);}
    return status;
}
