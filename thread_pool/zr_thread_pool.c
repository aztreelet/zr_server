
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "./zr_thread_pool.h"
#include "../util/util.h"
#include "../util/zr_ds.h"
#include "../http/http_conn.h"


#define ZR_SEM_WAIT(sem)    sem_wait(&sem)
#define ZR_SEM_POST(sem)    sem_post(&sem)
#define ZR_LOCK(locker)     pthread_mutex_lock(&locker)
#define ZR_UNLOCK(locker)   pthread_mutex_unlock(&locker)

/***************************/
/* In this project, the DATA_TYPE is HTTP_CONN_t. If you want use the pthrad_pool in project yourself,
 * please modify the DATA_TYPE.
 */
#define DATA_TYPE HTTP_CONN_t
/***************************/


ZR_THREAD_POOL_t *zr_thread_pool_create(int thread_number, int max_request)
{
    /* Set the default value */
    if (thread_number <= 0) {
        thread_number = 8;
        zr_printf(WARN, "Set thread_number <= 0, use default number = 8.");
    }
    if (max_request <= 0) {
        max_request = 1000;
        zr_printf(WARN, "Set max_request <= 0, use default number = 1000.");
    }

    ZR_THREAD_POOL_t *ztp = (ZR_THREAD_POOL_t *)malloc(sizeof(ZR_THREAD_POOL_t));

    ztp->m_stop = false;
    ztp->m_thread_number = thread_number;
    ztp->m_max_request = max_request;
    /* create the workqueue */
    ztp->m_workqueue = d_queue_create();
    /* init the mutex locker */
    pthread_mutex_init(&(ztp->m_workqueue_locker), NULL);

    /* Malloc the mem of thread`s array */
    ztp->m_threads = (pthread_t *)malloc(sizeof(pthread_t) * ztp->m_thread_number);
    if (!ztp->m_threads) {
        zr_printf(ERROR, "zr_thread_pool_create: malloc mem of m_threads.")
    }

    /* Create thread_number threads and set all detach */
    for (int i = 0; i < ztp->m_thread_number; ++i) {
        zr_printf(INFO, "create the %dth thread", i);
        if (pthread_create(ztp->m_threads + i, NULL, worker, ztp) != 0) {
            /* create false */
            free(ztp->m_threads);
            zr_printf(ERROR, "pthread_create %dth thread false.", i);
        }
        /* Set detach. The thread will auto free after run over in state detach */
        if (pthread_detach(ztp->m_threads[i])) {
            free(ztp->m_threads);
            zr_printf(ERROR, "pthread_detach %dth thread false.", i);
        }
    }

    return ztp;
}


void zr_thread_pool_destory(ZR_THREAD_POOL_t *ztp)
{
    free(ztp->m_threads);
    ztp->m_stop = true;
}

/* Add task into workqueue, the data can be anytype that defined by user */
/* The type Node shouldn't be modified, just define the data! */
/* In this project, the data is type HTTP_CONN_t. */
bool zr_thread_pool_append(ZR_THREAD_POOL_t *ztp, void *data)
{
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    /* Note: locker and sem */
    ZR_LOCK(ztp->m_workqueue_locker);
    if ( (d_queue_size(ztp->m_workqueue) > ztp->m_max_request) ) {
        ZR_UNLOCK(ztp->m_workqueue_locker);
        return false;
    }
    /* Enqueue */
    d_queue_push_back(ztp->m_workqueue, data);
    ZR_UNLOCK(ztp->m_workqueue_locker);
    /* sem + 1 */
    ZR_SEM_POST(ztp->m_workqueue_sem);

    return true;
}


void *worker(void *arg)
{
    //zr_printf(INFO, "In Worker runing by thread.");
    /* First trans type of arg, the type defined by user. */
    ZR_THREAD_POOL_t *ztp = (ZR_THREAD_POOL_t *)arg;
    
    zr_thread_pool_run(ztp);

    return ztp;
}

/* The thread is in an infinite loop, and if the queue is not empty (semaphore >0), a task is queued and processed */
void zr_thread_pool_run(ZR_THREAD_POOL_t *ztp)
{
    //zr_printf(DEBUG, "zr_thread_pool_run......");

    while ( !ztp->m_stop ) {
        /* wait the sem, then first lock */
        ZR_SEM_WAIT(ztp->m_workqueue_sem);
        ZR_LOCK(ztp->m_workqueue_locker);
        
        if (d_queue_is_empty(ztp->m_workqueue)) {
            /* No queue, unlock */
            ZR_UNLOCK(ztp->m_workqueue_locker);
            continue;
        }
        /* The Node in workqueue, it should include some data
         * Get the front node in queue.
         */
        struct Node *request_node;
        /* If we want to modify the val of request_node(its a pointer), add '&' */
        d_queue_pop_front(ztp->m_workqueue, &request_node);

        /* unlock */
        ZR_UNLOCK(ztp->m_workqueue_locker);

        if ( !request_node ) {
            continue;
        }

        hook_request_handler(request_node);
    }
}

// TODO 降低耦合，传进来一个处理函数的指针吧，正处理函数应该在http头文件中定义 
// 算了，先跑起来再说
void hook_request_handler(struct Node *request)
{
    /* 检查DATA_TYPE是否定义 */
#ifndef DATA_TYPE
    zr_printf(ERROR, "Please define the DATA_TYPE at the begining of this file!");
    return;
#endif

    /* Handle the request. */
    http_conn_process((DATA_TYPE *)(request->data));

    /*
     * The node`s data should be HTTP_CONN_t;
     * The process to handle the http_conn request;
     */
}

/* Just for test
 * $ gcc zr_thread_pool.c ../util/util.c ../util/zt_ds.h -lpthread
 */
#if 0
int main () 
{
    ZR_THREAD_POOL_t *ztp = zr_thread_pool_create(5, 5);

    struct Data data01 = { .a = 1 };
    struct Data data02 = { .a = 2 };
    struct Data data03 = { .a = 3 };

    /* The task append in queue, there will be a pthread get task from queue and handle it.
     * In hook_request_handler func, will print the data;
     */
    zr_thread_pool_append(ztp, (void *)(&data01));
    zr_thread_pool_append(ztp, (void *)(&data02));
    zr_thread_pool_append(ztp, (void *)(&data03));

    /* Note: for test should wait call the hook_request_handler.
     * The result print, maybe not 1,2,3 but in any order.
     */
    sleep(1);

    zr_thread_pool_destory(ztp);

    return 0;
}


#endif