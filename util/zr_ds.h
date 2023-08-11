/*
 * V1.0 基础数据结构实现
 */

#ifndef ZR_DS_H
#define ZR_DS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*********  doubly queueu  */
struct Node {
    void *data;
    struct Node *prev;
    struct Node *next;
};
/* The head and tail of d_queue */
typedef struct {
    struct Node *front;
    struct Node *rear;
} D_QUEUE_t;

D_QUEUE_t *d_queue_create();

/* isEmpty */
bool d_queue_is_empty(D_QUEUE_t *dq);

/* push the front node of d_queue */
void d_queue_push_front(D_QUEUE_t *dq, void *data);

/* Push a node in tail of d_queue */
void d_queue_push_back(D_QUEUE_t *dq, void *data);

/* Pop a node in head of d queue */
void d_queue_pop_front(D_QUEUE_t *dq, struct Node **node);

/* Pop a node in tail of d_queue */
void d_queue_pop_back(D_QUEUE_t *dq, struct Node **node);

/* Size */
int d_queue_size(D_QUEUE_t *dq);

/* free d_queue */
void d_queue_free(D_QUEUE_t *dq);

/********************************************/

/******** Just for test */
/* Parse and print data, use define it by self */
struct Data {
    int a;
};

void d_queue_print(D_QUEUE_t *dq);


/* The func should be define by user in different Data types. */
void hook_data_handler(void *data);

void data_print(void *data);



#endif