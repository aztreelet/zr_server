#### 双向队列实现

较基础的数据结构，直接看源码即可。

需要注意，源码中直接包含了测试代码。

**Notes**

* 双向不循环队列；
* data的结构自定义，此处为了测试定义为：`struct Data { int a };`，传data的地方注意需要转成`void *`类型的指针，具体在解析data时会转换回去；
* pop操作后只改变node的前后指针指向为NULL，但并不释放该node，因为其中的data还需要后续使用，在用户使用完毕后手动释放。（是否需要改进？）
* 最后三个函数中都要求将`void *`的data转换为自己定义的类型（可以优化，考虑使用宏定义实现）；
* hook的设计，如果别的函数需要处理队列中的数据，调用该函数即可，用户应在该函数中自定义处理过程；

以下是**测试函数**实现（已在源码中，可以参考使用）：

```c
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

其它：设计宏实现对data类型的转换（未实装）：
```c
tyepdef struct DATA YOUR_DATA_TYPE;
#define DATA_TYPE_2_VOID(datatype) (void*)(datatype)
#define VOID_2_DATA_TYPE(datatype) (YOUR_DATA_TYPE*)(datatype)
```