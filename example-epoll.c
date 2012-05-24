#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "adapters/epoll.h"
#include "hiredis.h"
#include "async.h"

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    if (reply == NULL) return;
    printf("k: %s\n", reply->str);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    int efd,max_events=10;
    efd = epoll_create(max_events);
    if (efd == -1) {
        printf("epoll_create() Error\n");
        return 1;
    }

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }

    redisEpollAttach(c,efd);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    redisAsyncCommand(c, NULL, NULL, "SET k 15");
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET k");
    redisAsyncCommand(c, NULL, NULL, "SET k 16");
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET k");
    redisAsyncCommand(c, NULL, NULL, "SET k 17");
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET k");
    redisAsyncCommand(c, NULL, NULL, "SET k 19");
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET k");
    redisAsyncDisconnect(c);
    redisEpollWait(efd,2,10,c);
    return 0;
}
