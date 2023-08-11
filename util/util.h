#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

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
#define LOG_LEVEL INFO

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


/*** 以下函数并没有被使用  */
/* 打印错误信息并退出 */
void errExit(const char*);

#endif
