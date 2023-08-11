#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

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
#include <sys/mman.h>
#include <sys/epoll.h>

#include "../util/util.h"
#include "../io/zr_io.h"
//#include "http_parse.h"
#include "http_conn.h"


/* 定义一些http协议的响应信�*/
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax o ris inherently impossible to satisfy.\n"; 
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requesred file.\n";

/* 网站文件根目�*/
//const char *doc_root = "/var/www/html";
const char *doc_root = "/home/zhai/www/html";


/*** some only one and include all should be static */
//static int epollfd;


/* INFO：切记，大差不差，先跑起来再优化 */

/* 
 * ~~× Get the only instance of http server, it includes everything~~
 * NONONO! is wrong.
 * Every user will have a http_conn_t!
 */
void http_conn_init(HTTP_CONN_t *http_connect, int *epollfd, int *user_count, int sockfd, const struct sockaddr_in *addr)
{   
    /* 创建静态http_connect */
    //static HTTP_CONN_t http_connect;
    /* 初始化该结构�*/
    /* 一些属性初始化 */
    http_connect->m_max_file_name_len = FILENAME_MAX;
    http_connect->m_read_buffer_size = READ_BUFFER_SIZE;
    http_connect->m_write_buffer_size = WRITE_BUFFER_SIZE;
    //method
    /* 事件组fd和用户总数初始�*/
    http_connect->m_epollfd = epollfd;
    http_connect->m_user_count = user_count;

    /*** 解析头相关变量初始化 */
    /* init the main state meachine state */
    http_connect->m_check_state = CHECK_STATE_REQUESTLINE;
    /* init which keep line */
    http_connect->m_linger = false;

    http_connect->m_method = GET;
    http_connect->m_url = 0;
    http_connect->m_version = 0;
    http_connect->m_content_length = 0;
    http_connect->m_host = 0;
    http_connect->m_start_line = 0;
    http_connect->m_check_idx = 0;
    http_connect->m_read_idx = 0;
    http_connect->m_write_idx = 0;
    /* init buffers */
    memset(http_connect->m_read_buffer, '\0', READ_BUFFER_SIZE);
    memset(http_connect->m_write_buffer, '\0', WRITE_BUFFER_SIZE);
    memset(http_connect->m_real_file, '\0', FILENAME_LEN);

    /* the server`s sockfd and addr info */
    http_connect->m_sockfd = sockfd;
    http_connect->m_address = *addr;
    /* add sockfd into epoll array and set et, oneshot , noblock */
    addfd(*epollfd, http_connect->m_sockfd, true, true);

}

/* In func hct_write, use it to init_var. */
void http_conn_init_var(HTTP_CONN_t *hct)
{
    hct->m_check_state = CHECK_STATE_REQUESTLINE;
    /* init which keep line */
    hct->m_linger = false;

    hct->m_method = GET;
    hct->m_url = 0;
    hct->m_version = 0;
    hct->m_content_length = 0;
    hct->m_host = 0;
    hct->m_start_line = 0;
    hct->m_check_idx = 0;
    hct->m_read_idx = 0;
    hct->m_write_idx = 0;
    /* init buffers */
    memset(hct->m_read_buffer, '\0', READ_BUFFER_SIZE);
    memset(hct->m_write_buffer, '\0', WRITE_BUFFER_SIZE);
    memset(hct->m_real_file, '\0', FILENAME_LEN);
}

/*
 * read the client data until no data to read or closed.
 * @return  true:read ok; false:error happen.  
 */
bool http_conn_read(HTTP_CONN_t *hct)
{
    if (hct->m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }

    int bytes_read = 0;
    while (true) {
        bytes_read = recv(hct->m_sockfd, hct->m_read_buffer + hct->m_read_idx, 
                        READ_BUFFER_SIZE - hct->m_read_idx, 0);
        if (bytes_read == -1) {
            /* The EW... is different name in other OS with EAGAIN */
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* try again */
                break;
            }

            zr_printf(DEBUG, "bytes_read is -1.");
            /* JSUT for test */
            perror("recv");     //recv: Socket operation on non-socket
            return false;
        }
        else if (bytes_read == 0) {
            zr_printf(DEBUG, "bytes_read is 0.");
            return false;
        }

        hct->m_read_idx += bytes_read;
    }
    return true;
}

/* Get a line data. */
char *http_conn_get_line(HTTP_CONN_t *http_connect)
{
    return (http_connect->m_read_buffer + http_connect->m_start_line);
}

/* 
 * Parse the http line, check if the reading line is complete or format of read text if right.
 *      ...
 * */
LINE_STATUS_t http_conn_parse_line(HTTP_CONN_t *http_connect)
{
    char temp;
    
    for ( ; http_connect->m_check_idx < http_connect->m_read_idx; ++(http_connect->m_check_idx)) {
        //逐字符解析，判断是否读到完整的行
        temp = http_connect->m_read_buffer[http_connect->m_check_idx];

        if (temp == '\r') {
            if (http_connect->m_check_idx + 1 == http_connect->m_read_idx) {
                //读到'\r'且以解析完所有已读数据，此时不能判断状态，需要继续读
                return LINE_OPEN;
            } else if (http_connect->m_read_buffer[http_connect->m_check_idx + 1] == '\n') {
                //成功读到完整的行，将回车换行符替换为0
                http_connect->m_read_buffer[(http_connect->m_check_idx)++] = '\0';
                http_connect->m_read_buffer[(http_connect->m_check_idx)++] = '\0';

                return LINE_OK;
            }
            //\r后面不是\n，说明存在语法问�            return LINE_BAD;

        } else if (temp == '\n') {
            if ((http_connect->m_check_idx > 1) && (http_connect->m_read_buffer[http_connect->m_check_idx - 1] == '\r')){
                http_connect->m_read_buffer[(http_connect->m_check_idx)++] = '\0';
                http_connect->m_read_buffer[(http_connect->m_check_idx)++] = '\0';

                return LINE_OK;
            }
            return LINE_BAD;
        }

    }   //end for

    /* 如果读完也未发现回车或换行符，表明数据未读完，需要继续读 */
    return LINE_OPEN;
}

/* 
 * Parse the request, return method if format is right, otherwise return BAD_REUQEST and so on.
 * 
 */
HTTP_CODE_t http_conn_parse_request_line(HTTP_CONN_t *http_connect, char *text)
{
    /* Find the pos of " \t", it means m_url --> " ". */
    http_connect->m_url = strpbrk(text, " \t");
    if (!http_connect->m_url) {
        /* 0 means not find " \t", is bad. */
        return BAD_REQUEST;
    }
    /* just like: *ht = \0, ht+1, sub the " " to "\0" */
    *(http_connect->m_url++) = '\0';

    char *method = text;
    if (strcasecmp(method, "GET") == 0) {
        http_connect->m_method = GET;
    }
    else {
        return BAD_REQUEST;
    }

    /* Set m_url point the next one pos of "\t". */
    http_connect->m_url += strspn(http_connect->m_url, " \t");
    
    http_connect->m_version = strpbrk(http_connect->m_url, " \t");
    if (!http_connect->m_version) {
        return BAD_REQUEST;
    }
    *(http_connect->m_version)++ = '\0';

    http_connect->m_version += strspn(http_connect->m_version, "\t");
    if (strcasecmp(http_connect->m_version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }

    if (strncasecmp(http_connect->m_url, "http://", 7) == 0) {
        http_connect->m_url += 7;
        http_connect->m_url = strchr(http_connect->m_url, '/');
    }

    if (!http_connect->m_url || http_connect->m_url[0] != '/') {
        return BAD_REQUEST;
    }

    http_connect->m_check_state = CHECK_STATE_HEADER;

    return NO_REQUEST;
}

/* Parse headers. */
HTTP_CODE_t http_conn_parse_header(HTTP_CONN_t *http_connect, char *text)
{
    /* Get the empty line, it means header has been parsed over. */
    if (text[0] == '\0') {
        /* If other content fllow, start parse centent, switch state meachine. */
        if (http_connect->m_content_length != 0) {
            http_connect->m_check_state = CHECK_STATE_CONETNT;
            return NO_REQUEST;
        }
        /* There is no content next, it means wo get a full request. */
        return GET_REQUEST;
    }
    /* Get a content and parse it. "Handle Connection" */
    else if ( strncasecmp(text, "Contention:", 11) == 0 ) {
        text += 11;
        text += strspn(text, " \t");
        if ( strcasecmp(text, "keep-alive") == 0 ) {
            http_connect->m_linger = true;
        }
    }
    /* Handle "Content-Length" */
    else if ( strncasecmp(text, "Content-Length:", 15) == 0 ) {
        text += 15;
        text += strspn(text, " \t");
    }
    /* Handle the Host */
    else if ( strncasecmp(text, "Host:", 5) == 0 ) {
        text += 5;
        text += strspn(text, " \t");
        http_connect->m_host = text;
    }
    else {
        zr_printf(WARN, "oop! unknow headers %s", text);
    }

    return NO_REQUEST;
}

/* Parse content. We dont really parse it, just determine if it has been read in its entirety.*/
HTTP_CODE_t http_conn_parse_content(HTTP_CONN_t *http_connect, char *text)
{
    if (http_connect->m_read_idx >= (http_connect->m_content_length + http_connect->m_check_idx)) {
        text[http_connect->m_content_length] = '\0';
        /* We set GET_REQUEST when reading content instead of waiting the complete of parsing content,
         * because we don't parse content.
         */
        return GET_REQUEST;
    }
    return NO_REQUEST;
}


/* The main status meachine process read. */
HTTP_CODE_t http_conn_process_read(HTTP_CONN_t *http_connect)
{
    LINE_STATUS_t line_status = LINE_OK;
    HTTP_CODE_t ret = NO_REQUEST;
    char *text = 0;

    while ( ((http_connect->m_check_state == CHECK_STATE_CONETNT) && (line_status == LINE_OK)) 
            || ( (line_status = http_conn_parse_line(http_connect)) == LINE_OK ) ) {
        text = http_conn_get_line(http_connect);     
        http_connect->m_start_line = http_connect->m_check_idx;
        zr_printf(INFO, "got 1 http line: %s", text);

        switch (http_connect->m_check_state) {
            case CHECK_STATE_REQUESTLINE: {
                ret = http_conn_parse_request_line(http_connect, text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = http_conn_parse_header(http_connect, text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                else if (ret == GET_REQUEST) {
                    return http_conn_do_request(http_connect);
                }
                break;
            }
            case CHECK_STATE_CONETNT: {
                ret = http_conn_parse_content(http_connect, text);
                if (ret == GET_REQUEST) {
                    return http_conn_do_request(http_connect);
                }
                line_status = LINE_OPEN;
                break;
            }
            default: {
                return INTERAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}

/*
 * Add data into write_buffer.
 */
bool http_conn_add_response(HTTP_CONN_t *hct, const char *format, ...)
{
    if (hct->m_write_idx >= WRITE_BUFFER_SIZE) {
        return false;
    }

    va_list arg_list;
    va_start(arg_list, format);
    /* Write into write_buffer. */
    int len = vsnprintf(hct->m_write_buffer + hct->m_write_idx, WRITE_BUFFER_SIZE - 1 - hct->m_write_idx,
                            format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - hct->m_write_idx)) {
        return false;
    }
    hct->m_write_idx += len;
    va_end(arg_list);

    return true;
}

/* Add stutas line, it's fixed. */
bool http_conn_add_status_line(HTTP_CONN_t *hct, int status, const char *title)
{
    return http_conn_add_response(hct, "%s %d %s\r\n", "HTTP/1.1", status, title); 
}

/* Add header line, it's fixed. */
bool http_conn_add_headers(HTTP_CONN_t* hct, int content_lenght)
{
    if (!http_conn_add_content_length( hct, content_lenght )) {
        zr_printf(ERROR, "add_content err!");
        return false;
    }

    if (!http_conn_add_linger(hct)) {
        zr_printf(ERROR, "add_linger err!");
        return false;
    }

    if (!http_conn_add_blank_line(hct)) {
        zr_printf(ERROR, "add_blank err!");
        return false;
    }

    return true;
}

bool http_conn_add_content_length(HTTP_CONN_t *hct, int content_length)
{
    return http_conn_add_response(hct, "Content-Length: %d\r\n", content_length);
}

bool http_conn_add_linger(HTTP_CONN_t *hct)
{
    return http_conn_add_response(hct,  "Connection: %s\r\n", (hct->m_linger == true) ? "keep-alive" : "close");
}

bool http_conn_add_blank_line(HTTP_CONN_t *hct)
{
    return http_conn_add_response(hct, "%s", "\r\n");
}

bool http_conn_add_content(HTTP_CONN_t *hct, const char *content)
{
    return http_conn_add_response(hct, "%s", content);
}


/* Based on the results of the server's processing of the HTTP request, 
 * decide what is returned to the client. 
 */
bool http_conn_process_write(HTTP_CONN_t *hct, HTTP_CODE_t ret)
{
    switch( ret ) {
        case INTERAL_ERROR: {
            http_conn_add_status_line(hct, 500, error_500_title);
            http_conn_add_headers(hct, strlen( error_500_form ));
            if ( !http_conn_add_content(hct, error_500_form) ) {
                return false;
            }
            break;
        }
        case BAD_REQUEST: {
            http_conn_add_status_line(hct, 400, error_400_title);
            http_conn_add_headers(hct, strlen( error_400_form ));
            if ( !http_conn_add_content(hct, error_400_form) ) {
                return false;
            }
            break;
        }
        case NO_REQUEST: {
            http_conn_add_status_line(hct, 404, error_404_title);
            http_conn_add_headers(hct, strlen( error_404_form ));
            if ( !http_conn_add_content(hct, error_404_form) ) {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST: {
            http_conn_add_status_line(hct, 403, error_403_title);
            http_conn_add_headers(hct, strlen( error_403_form ));
            if ( !http_conn_add_content(hct, error_403_form) ) {
                return false;
            }
            break;
        }
        case FILE_REQUEST: {
            //zr_printf(DEBUG, "mmap the file OK.");
            /* The file request need to be handled specifically. */
            http_conn_add_status_line(hct, 200, ok_200_title);

            if (hct->m_file_stat.st_size != 0) {
                http_conn_add_headers(hct, hct->m_file_stat.st_size);
                hct->m_iv[0].iov_base = hct->m_write_buffer;
                hct->m_iv[0].iov_len = hct->m_write_idx;
                hct->m_iv[1].iov_base = hct->m_file_address;
                hct->m_iv[1].iov_len = hct->m_file_stat.st_size;
                hct->m_iv_count = 2;

                return true;
            }
            else {
                const char *ok_string = "<html><body></body></html>";
                http_conn_add_headers(hct, strlen(ok_string));
                if ( !http_conn_add_content(hct, ok_string)) {
                    return false;
                }
            }

            break;
        }
        default: {
            return false;
        }
    }
    hct->m_iv[0].iov_base = hct->m_write_buffer;
    hct->m_iv[0].iov_len = hct->m_write_idx;
    hct->m_iv_count = 2;

    return true;
}


/* Write the http request. */
bool http_conn_write(HTTP_CONN_t *hct)
{
    int temp = 0;
    /* bytes have send. */
    int bytes_have_send = 0;
    /* bytes not send and need to send. */
    int bytes_to_send = hct->m_write_idx;

    if (bytes_to_send == 0) {
        /* =0 means not start read (not event IN), so set a EPOLLIN to let server append to thread pool. */
        modfd(*(hct->m_epollfd), hct->m_sockfd, EPOLLIN);

        /* reset some var in hct.
         */
        http_conn_init_var(hct);
        
        //zr_printf(INFO, "Check init in func hct_write, hct->read_idx = %d", hct->m_read_idx);
        return true;
    }

    /* Start write */
    while (1) {
        /* writev: Write some buf data into fd. */
        temp = writev(hct->m_sockfd, hct->m_iv, hct->m_iv_count);
        if (temp <= -1) {
            /* Error */
            if (errno == EAGAIN) {
                /* No space in write_buf. */
                modfd( *(hct->m_epollfd), hct->m_sockfd, EPOLLOUT );
                return true;
            }
            http_conn_unmap(hct);
            return false;
        }

        bytes_to_send -= temp;
        bytes_have_send += temp;
        if (bytes_to_send <= bytes_have_send) {
            /* Send success, determind which close connect by the hct->linger. */
            http_conn_unmap(hct);
            /* The conn request keep connect, reset the EPOLLIN. */
            if (hct->m_linger) {
                http_conn_init_var(hct);
                modfd(*(hct->m_epollfd), hct->m_sockfd, EPOLLIN);
                return true;
            }
            else {
                modfd(*(hct->m_epollfd), hct->m_sockfd, EPOLLIN);
                return false;
            }
        }
    }

}


/* We should parse the stat of file and mmap it to mem after get full request, 
 * and then ack the user that the file is useable.
 */
HTTP_CODE_t http_conn_do_request(HTTP_CONN_t *hct)
{
    strcpy(hct->m_real_file, doc_root);
    int len = strlen( doc_root );
    /* If ncopy is len(url) ? */
    strncpy(hct->m_real_file + len, hct->m_url, FILENAME_LEN - len - 1);

    if (stat(hct->m_real_file, &hct->m_file_stat) < 0) {
        /* The file is not exist.  */
        return NO_RESOURCE;
    }

    /*The file exist, test its attrs. */
    if ( !(hct->m_file_stat.st_mode & S_IROTH) ) {
        /* Others user no power to read it. */
        return FORBIDDEN_REQUEST;
    }

    if ( S_ISDIR(hct->m_file_stat.st_mode) ) {
        /* The file is a dir. */
        return BAD_REQUEST;
    }

    /* Open the file and mmap it to mem. */
    int fd = open(hct->m_real_file, O_RDONLY);
    /* @PROT_READ: Set only read for this mem.
     * @MAP_PRIVATE: Set private for other thread, the modify in this mem dont influence source file.
     */
    hct->m_file_address = (char *)mmap(0, hct->m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);
    return FILE_REQUEST;

}

/* Unmap */
void http_conn_unmap(HTTP_CONN_t *hct)
{
    if (hct->m_file_address) {
        munmap(hct->m_file_address, hct->m_file_stat.st_size);
        hct->m_file_address = 0;
    }
}


/* The func is entry of process, the thread in pool will call this func. */
void http_conn_process(HTTP_CONN_t *hct)
{
    HTTP_CODE_t read_ret = http_conn_process_read(hct);
    if (read_ret == NO_REQUEST) {
        /* *的优先级大于-> */
        //modfd(*hct->m_epollfd, hct->m_sockfd, EPOLLIN); 留下以作警示�        //zr_printf(DEBUG, "epollfd: %d", *(hct->m_epollfd));
        modfd(*(hct->m_epollfd), hct->m_sockfd, EPOLLIN);
        return;
    }

    bool write_ret = http_conn_process_write(hct, read_ret);
    if (!write_ret) {
        http_conn_close(hct);
    }

    modfd(*(hct->m_epollfd), hct->m_sockfd, EPOLLOUT);
}


void http_conn_close(HTTP_CONN_t *http_connect)
{
    if (http_connect->m_sockfd != -1) {
        removefd(*(http_connect->m_epollfd), http_connect->m_sockfd);
        /* For debug */
        zr_printf(INFO, "Close the client fd: %d, user count: %d", http_connect->m_sockfd, *(http_connect->m_user_count) - 1);
        http_connect->m_sockfd = -1;
        (*(http_connect->m_user_count))--;
    }
}
