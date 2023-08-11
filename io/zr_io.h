#ifndef __ZR_IO_H__
#define __ZR_IO_H__

#include <stdbool.h>

/* 将fd设置为非阻塞 */
int setnoblock(int fd);

/* 将fd和其事件添加到epoll事件表中
 * @ et:        是否开ET模式
 * @ oneshot：  是否注册EPOLLONESHOT事件
 * */
void addfd(int epollfd, int fd, bool et, bool oneshot);


/* Remove fd from epollfd */
void removefd(int epollfd, int fd);

/* Modify fd 
 * Add a ontshot and ev.
 */
void modfd(int epollfd, int fd, int ev);


/* 重置EPOLLONESHOT事件 */
void reset_oneshot(int epollfd, int fd);

/* 工作线程框架 
 * @ arg:        参数，需要强转成需要的类型
 * */
//void *worker(void *arg);

int zr_test_epoll_oneshot_loop();

#endif
