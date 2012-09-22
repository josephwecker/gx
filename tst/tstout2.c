#include "./gx.h"
#include "./gx_token.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>

gx_error_initialize(GX_ERROR);

int main(int argc, char **argv) {
    int stat;
    fork(); //fork(); fork(); fork();

    //dup2(STDERR_FILENO, STDERR_FILENO);
    int i,j;
    for(i = 0; i < 100; i++) {
        static char outstr[1024] = {0};
        char *p = outstr;
        unsigned int r1;
        pid_t pid = getpid();
        p = outstr;
        X (gx_dev_random(&r1, sizeof(r1), 0)) X_ERROR;
        int num = (r1 % 8) + 1;
        for(j=0; j < num; j++) p += sprintf(p, " -=%u=- ", pid);
        p += sprintf(p, "\n");
        for(j=0; j < num; j++) p += sprintf(p, " -=%u=- ", pid);
        p += sprintf(p, "\n\n");
        //fprintf(stderr, "%s\n\n", outstr);
        //p[0] = '\n';
        //p[1] = '\0';
        //write(STDERR_FILENO, outstr, strlen(outstr));
        static struct iovec iov[1];
        iov[0].iov_base = outstr;
        iov[0].iov_len  = strlen(outstr);
        if(writev(STDERR_FILENO, iov, 1) == -1) {
            perror("writev");
            return 1;
        }
        //gx_sleep(0,r1 & 0xffff);
    }
    wait(&stat);
    return 0;
}
