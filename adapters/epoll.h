#ifndef __HIREDIS_EPOLL_H__
#define __HIREDIS_EPOLL_H__
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include "../hiredis.h"
#include "../async.h"

typedef struct redisEpollEvents {
    redisAsyncContext *context;
    struct epoll_event event;
    int efd,fd;
} redisEpollEvents;



static void redisEpollAddRead(void *privdata) {
    printf("add read\n");
    redisEpollEvents *e = (redisEpollEvents*)privdata;
    e->event.data.fd = e->fd;
    e->event.events |= EPOLLIN ;
    epoll_ctl (e->efd, EPOLL_CTL_ADD, e->fd, &(e->event));    
}

static void redisEpollDelRead(void *privdata) {
    printf("del read\n");
    redisEpollEvents *e = (redisEpollEvents*)privdata;
    e->event.data.fd = e->fd;
    e->event.events &= !EPOLLIN ;
    epoll_ctl (e->efd, EPOLL_CTL_ADD, e->fd, &(e->event));    
}

static void redisEpollAddWrite(void *privdata) {
    printf("add write\n");
    redisEpollEvents *e = (redisEpollEvents*)privdata;
    e->event.data.fd = e->fd;
    e->event.events |= EPOLLOUT ;
    epoll_ctl (e->efd, EPOLL_CTL_ADD, e->fd, &(e->event));    
}

static void redisEpollDelWrite(void *privdata) {
    printf("del write\n");
    redisEpollEvents *e = (redisEpollEvents*)privdata;
    e->event.data.fd = e->fd;
    e->event.events &= !EPOLLOUT ;
    epoll_ctl (e->efd, EPOLL_CTL_ADD, e->fd, &(e->event));    
}

static void redisEpollCleanup(void *privdata) {
    printf("clean\n");
    redisEpollEvents *e = (redisEpollEvents*)privdata;
    epoll_ctl (e->efd, EPOLL_CTL_DEL, e->fd, NULL);    
    free(e);
}

static void redisEpollWait(int efd, int max_events, int timeout, redisAsyncContext *c){
    int i;
    int num_fds;

    struct epoll_event *events;
    events = calloc(1,max_events*sizeof(struct epoll_event));
    for(;;){
        num_fds = epoll_wait(efd, events, max_events, timeout);
        if(num_fds==0)
		return;
        for (i = 0; i < num_fds; i++) {
            if (events[i].events & EPOLLIN) {
                printf("READ\n");
                redisAsyncHandleRead(c);
            }
            else if (events[i].events & EPOLLOUT) {
                printf("WRITE\n");
                redisAsyncHandleWrite(c);
            }
	    else{
                printf("Unknown Event Type\n");
            }
        }
   }
}

static int redisEpollAttach(redisAsyncContext *rc, int efd)
{
    redisContext *c = &(rc->c);
    redisEpollEvents *e;

    /* Nothing should be attached when something is already attached */
    if (rc->ev.data != NULL)
        return REDIS_ERR;

    /* Create container for context and r/w events */
    e = (redisEpollEvents*)malloc(sizeof(*e));
    e->context = rc;
    e->efd = efd;
    e->fd = c->fd;
    e->event.data.fd = e->fd;
    e->event.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP| EPOLLIN| EPOLLOUT;

    /* Register functions to start/stop listening for events */
    rc->ev.addRead = redisEpollAddRead;
    rc->ev.delRead = redisEpollDelRead;
    rc->ev.addWrite = redisEpollAddWrite;
    rc->ev.delWrite = redisEpollDelWrite;
    rc->ev.cleanup = redisEpollCleanup;
    rc->ev.data = e;

    epoll_ctl (e->efd, EPOLL_CTL_ADD, e->fd, &(e->event));    
 
    return REDIS_OK;
}
#endif

