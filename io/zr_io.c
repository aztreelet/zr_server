/*
 * io复用实现文件
 * 2023-04-10   创建，写入设置noblock的函� * */

#include <stdio.h>
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
#include <sys/epoll.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>


#include "../http/http_conn.h"
#include "zr_io.h"
#include "../util/util.h"

#define MAX_EVENT_NUMBER 10
#define BUFFER_SIZE 1024

typedef struct {
    int epollfd;
    int sockfd;
} fds;


/* ��fd����Ϊ������ */
int setnoblock(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;

    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/* ��fd�����¼���ӵ�epoll�¼�����
 * @ et:        �Ƿ�ETģʽ
 * @ oneshot��  �Ƿ�ע��EPOLLONESHOT�¼���ע����¼���ǰ����ETģʽ
 * */
void addfd(int epollfd, int fd, bool et, bool oneshot) {
    struct epoll_event event;
    event.data.fd = fd;
    /* EPOLLRDHUPΪ�Զ˻����ӹر��¼� */
    event.events = EPOLLIN | EPOLLRDHUP;

    if (oneshot) {
        if (!et) {
            errExit("addfd oneshot but not et!");
        }
        event.events |= EPOLLET;
        event.events |= EPOLLONESHOT;
    } else if (et) {
        /* et true, oneshot false */
        event.events |= EPOLLET;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnoblock(fd);
}

/* Remove fd from epollfd */
void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/* Modify fd 
 * Add a ontshot and ev.
 */
void modfd(int epollfd, int fd, int ev)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}


/* ����EPOLLONESHOT�¼� */
void reset_oneshot(int epollfd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

#if 0
/* �����߳̿�� 
 * @ arg:        ��������Ҫǿת����Ҫ������
 * */
void *worker(void *arg) {
    int sockfd = ((fds*)arg)->sockfd;
    int epollfd = ((fds*)arg)->epollfd;
    printf("start new thread to recv data on fd: %d\n", sockfd);

    char buf[BUFFER_SIZE];
    memset(buf, '\0', BUFFER_SIZE);

    while (1) {
        int ret = recv(sockfd, buf, BUFFER_SIZE - 1,0);
        if (ret == 0) {
            close(sockfd);
            printf("foreiner closed the connection\n");
            break;
        } else if (ret < 0) {
            /* EAGAIN */
            if (errno == EAGAIN) {
                reset_oneshot(epollfd, sockfd);
                printf("read later\n");
                break;
            }
        } else {
            printf("get content: %s\n", buf);
            /* ����5s��ģ�����ݴ������ */
            sleep(5);
        }
    }

    printf("end thread receing data on fd: %d\n", sockfd);
}
#endif

/* main��ں���ֻ����һ�������������꣬��makefile����ʱָ�� */
#ifdef IO_MAIN
int main(int argc, char *argv[])
{
    //test_server(argc, argv);
    //test_readLine();
    //test_http_parse();
    zr_test_epoll_oneshot_loop(argc, argv);

    return 0;
}
#endif
