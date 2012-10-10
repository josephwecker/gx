#include <string.h>
#include "../gx.h"
#include "../gx_mqueue.h"

#define GX_MQUEUE_TEST_NAME "/gx-mq-test"

int main(int argc, char **argv) {
    int r_mqfd;
    int w_mqfd;

    char string[] = "Hello World!";
    char buffer[GX_MQUEUE_MSG_SIZE] = {0};

    unsigned prio = 0;

    _ (r_mqfd = gx_mq_open(GX_MQUEUE_TEST_NAME, O_CREAT | O_RDONLY | O_NONBLOCK, 0600)) _abort();
    _ (w_mqfd = gx_mq_open(GX_MQUEUE_TEST_NAME, O_CREAT | O_WRONLY | O_NONBLOCK, 0600)) _abort();

    _ (gx_mq_send(w_mqfd, string, strlen(string), prio))  _abort();
    _ (gx_mq_recv(r_mqfd, buffer, sizeof(buffer), &prio)) _abort();

    if(strcmp(string, buffer) != 0) {
        fprintf(stderr, "Warning: \"%s\" != \"%s\"\n", string, buffer);
    }

    _ (gx_mq_close(r_mqfd)) _abort();
    _ (gx_mq_close(w_mqfd)) _abort();

    _ (gx_mq_unlink(GX_MQUEUE_TEST_NAME)) _abort();

    return 0;
}
