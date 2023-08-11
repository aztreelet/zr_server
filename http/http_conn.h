#ifndef HTTP_CONN_H
#define HTTP_CONN_H

/* 
 * 时间�0230625
 * 功能：处理http连接请求
 * 版本：V1.0
 * 参考：游双P304
 * */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/uio.h>

//#include "http_parse.h"


//文件名的最大长�#define FILENAME_LEN        200
#define READ_BUFFER_SIZE    2048
#define WRITE_BUFFER_SIZE   1024

/* 定义主状态机，表示状态：
 *      1. 当前正在分析请求� *      2. 正在分析头部字段
 *      1 --> 2
 * */
typedef enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0,
    CHECK_STATE_HEADER,
    CHECK_STATE_CONETNT,
} CHECK_STATE_t;

/* 从状态机，表示状态：
 *      1. 读到完整� *      2. 行出� *      3. 正在读取行数� * */
typedef enum LINE_STATUS {
    LINE_OK = 0,
    LINE_BAD,
    LINE_OPEN,
} LINE_STATUS_t;

/* 
 * 从状态机，http头状态，表示状态：
 *      1. 请求不完整，需要继续读� *      2. 无请求的文件
 *      3. 获得完整的客户请� *      4. 客户请求有语法错� *      5. 客户对资源无访问权限
 *      6. 服务器内部错� *      7. 客户端已经关闭连� * */
typedef enum HTTP_CODE {
    NO_REQUEST = 0,
    NO_RESOURCE,
    GET_REQUEST,
    BAD_REQUEST,
    FILE_REQUEST,
    FORBIDDEN_REQUEST,
    INTERAL_ERROR,
    CLOSED_CONNECTION,
}HTTP_CODE_t;

//请求方法
//本程序只支持GET
typedef enum {
    GET = 0,
    POST,
    HEAD,
    PUT,
    DELETE,
    //TRACE,
    OPTIONS,
    CONNECT,
    PATCH
} METHOD_t;

typedef struct
{
    //最大文件长度，=FILENAME_LEN
    int       m_max_file_name_len;
    //读缓冲区大小
    int       m_read_buffer_size;
    int       m_write_buffer_size;

    /* The epollfd and user_count should be shared ro every http_conne,
     * so set it global instead of set in this struct. 
      */
    int             *m_epollfd;
    int             *m_user_count;

    int                 m_sockfd;
    struct sockaddr_in  m_address;
    char                m_read_buffer[ READ_BUFFER_SIZE ];
    int                 m_read_idx;
    int                 m_check_idx;
    /* 正在解析的行的起始位�*/
    int                 m_start_line;
    char                m_write_buffer[ WRITE_BUFFER_SIZE ];
    int                 m_write_idx;

    /*** 状态机状态相�*/
    /* the main statemeahine`s state */
    CHECK_STATE_t       m_check_state;
    /* the request method */
    METHOD_t            m_method;
    
    /* 完整路径名，包含path+filename */
    char    m_real_file[ FILENAME_LEN ];
    /* 以下都是char*类型，指向http报文中响应的位置 */
    char*   m_url;
    char*   m_version;
    char*   m_host;
    //http请求的消息体的长�    int     m_content_length;
    //HTTP请求是否要求保持连接
    bool    m_linger;

    /* 共享内存相关 */
    //文件被mmap的内存地址
    char*   m_file_address;
    //目标文件状�    struct stat m_file_stat;
    /* 使用writev分块写，相关如下�/
    struct iovec    m_iv[2];
    //总块�    int             m_iv_count;

} HTTP_CONN_t;


/************** 函数接口  ******************/
/* Init and set sockfd noblock.
 * @http_connect: addr of HTTP_CONN_T that has be malloced.
 * @epollfd, user_count: Note that they are points, because the var used by all users.
 */
void http_conn_init(HTTP_CONN_t *http_connect, int *epollfd, int *user_count, int sockfd, const struct sockaddr_in *addr);

/* In func hct_write, use it to init_var. */
void http_conn_init_var(HTTP_CONN_t *hct);

/* Read data from client connect until no data to read. */
bool http_conn_read(HTTP_CONN_t *http_connect);

/* Write the http request. */
bool http_conn_write(HTTP_CONN_t *hct);


/* Get a line data. */
char *http_conn_get_line(HTTP_CONN_t *http_connect);


LINE_STATUS_t http_conn_parse_line(HTTP_CONN_t *http_connect);

HTTP_CODE_t http_conn_parse_request_line(HTTP_CONN_t *http_connect, char *text);

/* Parse headers. */
HTTP_CODE_t http_conn_parse_header(HTTP_CONN_t *http_connect, char *text);

/* Parse content. We dont really parse it, just determine if it has been read in its entirety.*/
HTTP_CODE_t http_conn_parse_content(HTTP_CONN_t *http_connect, char *text);

/* The main state meachine process read. */
HTTP_CODE_t http_conn_process_read(HTTP_CONN_t *http_connect);

/* Write the http request msg. */
bool http_conn_process_write(HTTP_CONN_t *hct, HTTP_CODE_t ret);


/* We should parse the stat of file and mmap it to mem after get full request, 
 * and then ack the user that the file is useable.
 * Mmap
 */
HTTP_CODE_t http_conn_do_request(HTTP_CONN_t *hct);
/* Unmap */
void http_conn_unmap(HTTP_CONN_t *hct);

/* The func is entry of process, the thread in pool will call this func. */
void http_conn_process(HTTP_CONN_t *hct);

void http_conn_close(HTTP_CONN_t *http_connect);

// TODO, the prefix should be modify later.
/* Some basic functions. */
bool http_conn_add_response(HTTP_CONN_t *hct, const char *format, ...);
bool http_conn_add_status_line(HTTP_CONN_t *hct, int status, const char *title);
bool http_conn_add_headers(HTTP_CONN_t *hct, int content_lenght);
bool http_conn_add_content_length(HTTP_CONN_t *hct, int content_length);
bool http_conn_add_linger(HTTP_CONN_t *hct);
bool http_conn_add_blank_line(HTTP_CONN_t *hct);
bool http_conn_add_content(HTTP_CONN_t *hct, const char *content);

#endif
