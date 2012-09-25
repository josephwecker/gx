#include "./gx.h"
#include "./gx_token.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>

gx_error_initialize(GX_ERROR);

int main(int argc, char **argv) {
    int stat;
    fork(); fork(); fork(); fork();

    //dup2(STDERR_FILENO, STDERR_FILENO);
    int i,j;
    char outstr[1024] = {0};
    char *p = outstr;
    pid_t pid = getpid();
    for(i = 0; i < 100; i++) {
        unsigned int r1;
        p = outstr;
        _ (gx_dev_random(&r1, sizeof(r1), 0)) _error();
        int num = (r1 % 8) + 1;
        for(j=0; j < num; j++) p += sprintf(p, " -=%u=- ", pid);
        p += sprintf(p, "\n");
        for(j=0; j < num; j++) p += sprintf(p, " -=%u=- ", pid);
        fprintf(stderr, "%s\n\n", outstr);
        //gx_sleep(0,000,r1 & 0xffff);
        //p[0] = '\n';
        //p[1] = '\0';
        //write(STDERR_FILENO, outstr, strlen(outstr));
        //writev(STDERR_FILENO, (struct iovec[]){{outstr,strlen(outstr)}}, 1);
    }
    wait(&stat);
    return 0;
}

