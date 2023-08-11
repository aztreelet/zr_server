**万能队列实现**

```c
/* zr_ds.h */
#ifndef ZR_DS_H
#define ZR_DS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*********  doubly queueu  */
/* The data can be any types that defined by user. */
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
```

**Readme**
* 双向不循环队列；
* data的结构自定义，此处为了测试定义为：`struct Data { int a };`，传data的地方注意需要转成`void *`类型的指针，具体在解析data时会转换回去；
* pop操作后只改变node的前后指针指向为NULL，但并不释放该node，因为其中的data还需要后续使用，在用户使用完毕后手动释放。（是否需要改进？）
* 最后三个函数中都要求将`void *`的data转换为自己定义的类型（可以优化，考虑使用宏定义实现）；
* hook的设计很巧妙，如果别的函数需要处理队列中的数据，调用该函数即可，请用户在该函数中设置处理过程；

以下是函数实现：

```c
/* zr_ds.c */

#include <stdbool.h>

#include "zr_ds.h"

D_QUEUE_t *d_queue_create()
{
    D_QUEUE_t *dq = (D_QUEUE_t*)malloc(sizeof(D_QUEUE_t));
    dq->front = NULL;
    dq->rear = NULL;

    return dq;
}

bool d_queue_is_empty(D_QUEUE_t *dq)
{
    if (dq->front == NULL && dq->rear == NULL) {
        return true;
    }
    return false;
}

void d_queue_push_front(D_QUEUE_t *dq, void *data)
{
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    new_node->data = data;
    new_node->prev = NULL;
    new_node->next = dq->front;

    if (dq->front == NULL) {
        dq->rear = new_node;
    }
    else {
        dq->front->prev = new_node;
    }

    dq->front = new_node;
}

/* Push a node in tail of d_queue */
void d_queue_push_back(D_QUEUE_t *dq, void *data)
{
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    new_node->data = data;
    new_node->next = NULL;
    new_node->prev = dq->rear;

    if (dq->rear == NULL) {
        dq->front = new_node;
    }
    else {
        dq->rear->next = new_node;
    }

    dq->rear = new_node;
}

/* Pop a node in head of d queue */
/* node is elem addr of pop */
/* Should return node addr instead of data type addr, because user will free node */
void d_queue_pop_front(D_QUEUE_t *dq, struct Node **node)
{
    if (dq->front == NULL) {
        printf("Error: pop_front empty.\n");
        return;
    }
    /* The data will be free in the func, so outside cant get the data, is Error */
    //data = dq->front->data;
    //((struct Data *)data)->a = ((struct Data *)(dq->front->data))->a;

    //node = &(dq->front);
    *node = dq->front;
    if (dq->front == dq->rear) {
        dq->front = NULL;
        dq->rear = NULL;
    }
    else {
        dq->front = dq->front->next;
        dq->front->prev = NULL;
    }

    /* Dont free it, the user should free it after using it */
    //free(temp);
}

/* Pop a node in tail of d_queue, the elem dont be released after pop
 * user should free it */
void d_queue_pop_back(D_QUEUE_t *dq, struct Node **node)
{
    if (dq->rear == NULL) {
        printf("Error: pop_back empty.\n");
        return;
    }
    /* the data will be free */
    //((struct Data *)data)->a = ((struct Data *)(dq->rear->data))->a;


    *node = dq->rear;
    if (dq->front == dq->rear) {
        dq->front = NULL;
        dq->rear = NULL;
    }
    else {
        dq->rear = dq->rear->prev;
        dq->rear->next = NULL;
    }
}


/* Size */
int d_queue_size(D_QUEUE_t *dq)
{
    int cnt = 0;
    struct Node *cur = dq->front;

    while (cur != NULL) {
        cnt++;
        cur = cur->next;
    }
    return cnt;
}


/* free d_queue */
void d_queue_free(D_QUEUE_t *dq)
{
    struct Node *cur = dq->front;

    while(cur != NULL) {
        struct Node *temp = cur;
        cur = cur->next;
        free(temp);
    }
    free(dq);
}

/* Parse and print data, use define it by self, the func own to Data */
void d_queue_print(D_QUEUE_t *dq)
{
    struct Node *cur = dq->front;

    int i = 1;
    while (cur != NULL) {
        void *cur_data = cur->data;
        /* data handler */
        hook_data_handler(cur_data);

        cur = cur->next;
    }
}

/* The hook of data_handle */
void hook_data_handler(void *data)
{
    data_print(data);
}

void data_print(void *data)
{
    struct Data * temp = (struct Data *)data;
    printf("The data = %d\n", temp->a);
}


/* Just for test 
 * $ gcc zr_ds.c 
 */
//#define TEST
#ifdef TEST
int main()
{   
    D_QUEUE_t *dq = d_queue_create();

    struct Data data01 = {10};
    struct Data data02 = {20};
    struct Data data03 = {30};

    d_queue_push_back(dq, (void*)&data01);
    d_queue_push_back(dq, (void*)&data02);
    d_queue_push_back(dq, (void*)&data03);

    printf("The size = %d\n", d_queue_size(dq));
    d_queue_print(dq);

    struct Node *node;
    d_queue_pop_front(dq, &node);
    data_print(node->data); // 输出：Front data: 10
    printf("The size = %d\n", d_queue_size(dq));
    
    /* The data has uesd over, free it
     * Though not free, but the 'next' and 'rear' of node has changed, the size always currect.
     */
    free(node);

    d_queue_pop_back(dq, &node);
    data_print(node->data); // 输出：Front data: 30
    free(node);
    printf("The size = %d\n", d_queue_size(dq));

    d_queue_print(dq);

    d_queue_free(dq);

    return 0;
}

#endif
```

* 请看main函数，熟悉流程；

参考：设计宏实现对data类型的转换（未实装）：
```c
tyepdef struct DATA DATA_TYPE;
#define DATA_TYPE_2_VOID(datatype) (void*)(datatype)
#define VOID_2_DATA_TYPE(datatype) (DATA_TYPE*)(datatype)
```