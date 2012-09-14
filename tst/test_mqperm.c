#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#define  QUEUE_NAME  "/test_queue"
#define  MAX_SIZE    1024
#define  MSG_STOP    "exit"
#define CHECK(x) do{if(!(x)){printf("%s:%d: ",__func__,__LINE__);perror(#x);exit(-1);}} while(0)

pid_t this_pid;

int client() {
    fork();
    mqd_t mq;
    struct mq_attr attr;
    char buffer[MAX_SIZE + 1];
    int must_stop = 0;
    sleep(1);

    char mqname[256] = {0};
    snprintf(mqname, 256, "%s%u", QUEUE_NAME, (unsigned int)this_pid);
    mq = mq_open(mqname, O_RDONLY);
    CHECK((mqd_t)-1 != mq);
    printf("\n:Client: Established connection to %s - pulling off queueu:\n", mqname);
    fflush(stdout);
    do {
        ssize_t bytes_read;
        bytes_read = mq_receive(mq, buffer, MAX_SIZE, NULL);
        CHECK(bytes_read >= 0);
        buffer[bytes_read] = '\0';
        if (! strncmp(buffer, MSG_STOP, strlen(MSG_STOP))) must_stop = 1;
        else printf(" :%u", (unsigned char)buffer[0]);
        fflush(stdout);
        //else printf("Received: %s\n", buffer);
    } while (!must_stop);
    printf("\n\n:Client: Pulled everything up to exit. Exiting\n");
    fflush(stdout);
    CHECK((mqd_t)-1 != mq_close(mq));
    mq_unlink(mqname);
    return 0;
}

int server() {
    mqd_t mq;
    struct mq_attr attr;
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    //char buffer[MAX_SIZE];
    char mqname[256] = {0};
    snprintf(mqname, 256, "%s%u", QUEUE_NAME, (unsigned int)this_pid);
    mq = mq_open(mqname, O_CREAT | O_WRONLY | O_NONBLOCK, 0644, &attr);
    CHECK((mqd_t)-1 != mq);
    printf("\n:Server: Established connection to %s\n", mqname);
    fflush(stdout);

    int i;
    for(i=0; i < 50; i++) {
        //int sz = snprintf(buffer, MAX_SIZE, "%d, oh yeah\n", i);
        //if(sz > 0) CHECK(0 <= mq_send(mq, buffer, (size_t)(sz + 1), i));
        //CHECK(0 <= mq_send(mq, (char *)(&i), sizeof(i), i));
        int res;
do_mq_send:
        res = mq_send(mq, (char *)(&i), sizeof(i), i);
        if(res == -1) {
            switch(errno) {
                case EINTR:  goto do_mq_send;
                case EAGAIN: usleep(1000);
                             goto do_mq_send;
                default:     CHECK(0);
            }
        }
        printf(" %u>", (unsigned int)i);
        fflush(stdout);
    }
    mq_send(mq, "exit", 5, 0);
    printf("\n:Server: Everything sent to message queue. Going to sleep for a bit\n");
    fflush(stdout);

    /*printf("Send to server (enter \"exit\" to stop it):\n");
    do {
        printf("> ");
        fflush(stdout);
        memset(buffer, 0, MAX_SIZE);
        fgets(buffer, MAX_SIZE, stdin);
        CHECK(0 <= mq_send(mq, buffer, MAX_SIZE, 0));
    } while (strncmp(buffer, MSG_STOP, strlen(MSG_STOP)));*/
    CHECK((mqd_t)-1 != mq_close(mq));
    CHECK((mqd_t)-1 != mq_unlink(mqname));
    wait();
    //sleep(4);
    return 0;
}

int main(int argc, char **argv) {
    pid_t pid;
    this_pid = getpid();
    pid = fork();
    if(pid) return server();
    else    return client();
}

