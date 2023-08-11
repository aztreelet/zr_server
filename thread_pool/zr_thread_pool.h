#ifndef ZR_THREAD_POOL_H
#define ZR_THREAD_POOL_H

#include <pthread.h>
#include <semaphore.h>

#include "../util/zr_ds.h"

typedef struct {
    int m_thread_number;    /* the num of threads */
    int m_max_request;      /* the max num of request queue (thread array) */
    pthread_t *m_threads;   /* the pointer of thread array */
    D_QUEUE_t *m_workqueue; /* the request queue */
    //locker ...            /* the locker in c++, but use mutex_lock.h in c */
    //sem
    bool m_stop;            /* if stop the thread */
     
    /* ptherad_mutex, protect the work_queue */
    pthread_mutex_t m_workqueue_locker;
    /* sem, determind if there is work needs to be processed */
    sem_t m_workqueue_sem;

} ZR_THREAD_POOL_t;

/* Return a  thread_pool with thread_number, max_request; */
ZR_THREAD_POOL_t *zr_thread_pool_create(int thread_number, int max_request);

void zr_thread_pool_destory(ZR_THREAD_POOL_t *ztp);

bool zr_thread_pool_append(ZR_THREAD_POOL_t *ztp, void *data);

/* The func used in pthread_create, the fixed formatting, 
 * the return could be NULL or anything */
void *worker(void *arg);

/* Pop a thread and let it working. */
void zr_thread_pool_run(ZR_THREAD_POOL_t *ztp);


void hook_request_handler(struct Node *request);



#endif
