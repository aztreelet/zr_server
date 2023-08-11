/* 
 * 一些功能性函数
 * */
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
 

/*** 以下函数并没有被使用  */
void errExit(const char *str)
{
	printf("err: %s\n", str);
	exit(-1);
}