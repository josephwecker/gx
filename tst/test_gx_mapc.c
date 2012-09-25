#include "../gx.h"
gx_error_initialize(GX_DEBUG);  // Should be able to remove later

#include <unistd.h>
#include "../gx_error.h"
#include "../gx_mapc.h"
#include "../gx_event.h"

gx_eventloop_declare  (mapc_events, 1, 1);
gx_eventloop_implement(mapc_events, 1, 1);

void writer() {
    int i;
    mapc *testm;
    char msg[] = "something blah blah";

    _N( testm = mapc_open("tmp/writer-file.dat", MAPC_WRITE | MAPC_VOLATILE) ) _eexit();
    gx_sleep(1);
    for(i=0; i < 5; i++) {
        gx_sleep(0,500);
        log_info("/CHILD/ writing some stuff");
        mapc_write(testm, msg, sizeof(msg));
    }
    gx_sleep(1);
}

void reader() {
    mapc *testm;
    gx_eventloop_init(mapc_events);
    gx_sleep(0,500);
    _N( testm = mapc_open("tmp/writer-file.dat", MAPC_READ) ) _eexit();
}

int main(int argc, char **argv) {
    pid_t pid;
    int status = 0;
    _ ( pid = fork() ) _eexit();
    if(pid){writer();}
    else   {reader(); waitpid(pid, &status, 0);}
    return status;
}
