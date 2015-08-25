#ifndef _EVENT_MSGQUEUE_H_
#define _EVENT_MSGQUEUE_H_

#include <event.h>

#ifdef __cplusplus
extern "C" {
#endif

struct event_msgqueue;

struct event_msgqueue *msgqueue_new(struct event_base *, unsigned int, void (*)(void *, void *), void *);
int msgqueue_push(struct event_msgqueue *, void *);
unsigned int msgqueue_length(struct event_msgqueue *);
void msgqueue_destroy(struct event_msgqueue*);

#ifdef __cplusplus
}
#endif

#endif

