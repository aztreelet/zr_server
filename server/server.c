/*
 * v1.0 20230707 server主程序
 * */
#include <asm-generic/socket.h>
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
#include <pthread.h>
#include <assert.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>

#include "../util/util.h"
#include "../util/zr_ds.h"
#include "../thread_pool/zr_thread_pool.h"
#include "../http/http_conn.h"
#include "../io/zr_io.h"
#include "server.h"

#define MAX_FD              65535
#define MAX_EVENT_NUMBER    10000

#define BACK_LOG 50
#define BUF_SIZE 128

/* The global var */
int g_epollfd;
int g_user_count;


/* is_start default is true */
void addsig(int sig, void(handler)(int), bool is_restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (is_restart) {
        sa.sa_flags |= SA_RESTART;
    }
    /* Add all signal to mask, dont use signal`s default handle method */
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}



/* The main server. */
int zr_server(const char *ip, const char *s_port)
{
    zr_printf(INFO, "Start the zr_server...");

    int port = atoi(s_port);

    /* ignore the SIGPIPE */
    addsig( SIGPIPE, SIG_IGN, true);

    /* malloc the thread poll */
    ZR_THREAD_POOL_t *ztp = zr_thread_pool_create(0, 0);
    if (ztp == NULL) {
        return -1;
    }
    
    /* Assign a http_conn instance to all client, but not init. */
    /* Init it when new connect coming and get a http_conn_t. */
    HTTP_CONN_t *users = (HTTP_CONN_t *)malloc(sizeof(HTTP_CONN_t) * MAX_FD); 
    assert(users);
    g_user_count = 0;

    /* Create listenfd socket and set delay close. */
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    /* 
     * struct linger {
     *     int l_onoff;		//是否延迟关闭
     *     int l_linger;	//延迟关闭时间
     * }; 
     */
    struct linger tmp = { 1, 0 };   //打开延迟关闭开关，但是时间设置为0，也就是说不延迟
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    /* Set addr's ip and port */
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    /* Bind */
    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    /* Listen */
    ret = listen(listenfd, 5);
    assert(ret >= 0);

    /* Set epoll */
    /**  */
    struct epoll_event events[ MAX_EVENT_NUMBER ];
    g_epollfd = epoll_create(5);
    assert(g_epollfd != 0);
    addfd(g_epollfd, listenfd, true, false);

    /* Start the main loop... */
    while (true) {
        /* The number of event happening */
        int number = epoll_wait(g_epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < -1) && (errno != EINTR)) {
            zr_printf(ERROR, "epoll failure");
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            /* The fd is server's listenfd, means that new connect coming. */
            if (sockfd == listenfd) {
                /* Set new client. */
                struct sockaddr_in client_address;
                socklen_t client_addr_len = sizeof(client_address);

                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addr_len);
                if (connfd < 0) {
                    zr_printf(WARN, "errno is %d", errno);
                    continue;
                }

                /* Ensure is new connect, but first should check user number. */
                if (g_user_count >= MAX_FD) {
                    zr_printf(WARN, "Internal server busy, connfd is %d", connfd);
                    continue;
                }

                /* Init the http_conn. */
                http_conn_init( &(users[connfd]), &g_epollfd, &g_user_count, connfd, &client_address ); 
                g_user_count++;
                zr_printf(INFO, "New client fd %d connect, user count: %d", connfd, g_user_count);

            }
            /* New event come but err. */
            else if ( events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR) ) {
                /* Somer err, close connect... */
                http_conn_close(&users[sockfd]);
            }
            /* New pollin event. */
            else if ( events[i].events & EPOLLIN ) {
                if (http_conn_read(&users[sockfd])) {
                    /* Append a data with type HTTP_CONN_t into ztp. */
                    zr_thread_pool_append(ztp, (void*)(users + sockfd));
                }
                else {
                    zr_printf(DEBUG, "read err, close");
                    http_conn_close(&users[sockfd]);
                }
            }
            /* New pollout event. */
            else if ( events[i].events & EPOLLOUT ) {
                zr_printf(DEBUG, "write to fd.");
                if ( !http_conn_write(&users[sockfd]) ) {
                    http_conn_close(&users[sockfd]);
                }
            }
            else {

            }
        }

    }

    /* Close all fd */
    close(g_epollfd);
    close(listenfd);
    free(users);
    free(ztp);

    return 0;
}


int main(int argc, char *argv[])
{
    /* This is for test */
    //zr_server("192.168.1.123", "6667");
 
    if (argc < 3) {
        zr_printf(ERROR, "Please input ip and port !!!");
        return -1;
    }

    zr_server(argv[1], argv[2]);

    return 0;
}