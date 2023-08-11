# util

分为两部分：

* 常用工具实现
* 常用数据结构实现

介绍一下功能函数；

## 常用工具实现

### 日志打印简单实现

#### 简介

**文件**：`util/util.h `;
**功能**：使用宏定义实现，分级别和颜色打印。如测试时设置打印级别为DEBUG，然后打印信息时设置好打印级别，在正式发布时将总的级别改为INFO或者WARN后，程序中DEBUG级别的打印将不再显示；
**代码**：

``` c
typedef enum Level
{
	OFF = 0,
	FATAL,
	ERROR,
	WARN,
	INFO,
	DEBUG,
	TRACE,
} Level_t;

//设置日志等级
#define LOG_LEVEL DEBUG

/* 设置字符的颜色 */
#define ESC     "\033["
#define RED     ESC "01;31m"
#define YELLOW  ESC "01;33m"
#define GREEN   ESC "01;32m"
#define BLUE    ESC "01;34m"
#define CLR     ESC "0m"

#define zr_printf(level, format, ...) \
    if(level <= LOG_LEVEL) { \
        if(level==INFO) { \
            fprintf(stderr, "%s[%s]\t[%s:%s:%d] " format "%s\n", \
                YELLOW, #level, __func__, __FILE__, __LINE__, ##__VA_ARGS__, CLR ); \
        } \
        else if (level==WARN) { \
            fprintf(stderr, "%s[%s]\t[%s:%s:%d] " format "%s\n", \
                BLUE, #level, __func__, __FILE__, __LINE__, ##__VA_ARGS__, CLR ); \
        } else if (level==ERROR) { \
            fprintf(stderr, "%s[%s]\t[%s:%s:%d] " format "%s\n", \
                RED, #level, __func__, __FILE__, __LINE__, ##__VA_ARGS__, CLR ); \
        } else { \
            fprintf(stderr, "[%s]\t[%s:%s:%d] " format "\n", \
                #level, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
        } \
    }
```

#### 实现思路

使用enum定义需要的日志等级；

分级打印部分就是if-else的判断，具体看源码；

 **需求分析：**

1. 分级别打印，不同级别不同颜色；
2. 出错时打印出当前文件、函数、行号等信息；
3. 支持自定义信息格式化输出；

针对以上讨论实现思路。

##### 详细信息打印和自定义输出

需要打印出当前的文件、函数和行数信息，使用c或c++中的系统预定义宏实现：

```text
__func__   此宏展开当前函数的名称作为一个字符串字面值；
__FILE__  同理，展开当前源文件名称；
__LINE__  同理，展开当前代码行号；
##__VA_ARGS__  ##用于处理可变数量的宏参数；
```

自定义输出是通过参数`format, ...`实现的，等同于printf函数中`"%d%s", i, str`这种格式，`...`对应后面的一个或多个变量；

有了以上函数，如何**拼接输出**，使用fprintf函数：

```c
 fprintf(stderr, "[%s]\t[%s:%s:%d] " format "\n", \
                #level, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
```

Notes：

* 在宏定义中，换行时结尾需要`\`，且其后面不能有空格；
* 我们将信息通过stderr输出，与stdout区分开来；
* 已经添加了换行，实际使用时不需要在手动加换行符；



##### 设置不同颜色

使用ANSI转义序列实现，用于控制终端中文本颜色、背景色等，这里只设置文本颜色，例如输出红色的“Hello world！”：

```c
// \033[0;31m  红色
// \033[0m  重置所有颜色
printf("\033[0;31mHello, World!\033[0m\n");
```

多种颜色，每次打印这样写比较麻烦，颜色的开头和结尾都是一样的，所以我们使用宏定义实现，具体颜色的宏定义见源码部分，比较简单，这里不展开叙述。

需要注意的是把他们加到fprintf函数中时，注意宏定义以及格式化字符（`%s`）的添加位置；

```c
if(level==INFO) { \
            fprintf(stderr, "%s[%s]\t[%s:%s:%d] " format "%s\n", \
                YELLOW, #level, __func__, __FILE__, __LINE__, ##__VA_ARGS__, CLR ); \
        } \
```



## zr_ds-常用数据结构实现

### 双向队列

讲道理单向就够了（主要是c++中list是个双向的，然后还有从队头或者队尾出的接口，就仿照着写了...）

这个基础的双向队列没有什么好说的，可自行阅读源码或者查阅资料，所有接口如下：

```c
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
/* create */
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
```

**Notes：**

* 此双向队列的实现是通用的，使用了 `void*` 类型指针保存用户自定义类型的数据，在处理数据时先强转回指定类型在进行处理，（例如对于此程序，data将会是HTTP_CON_t类型数据）；
* 我们会在使用时要求定义一个宏 `DATA_TYPE`，具体使用方法会在线程池实现讲解；









