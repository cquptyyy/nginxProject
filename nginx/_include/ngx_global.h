#ifndef __NGX__GLOBAL_H__
#define __NGX__GLOBAL_H__

#include <unistd.h>
#include <signal.h>
#include <cstdio>
#include "ngx_c_socket.h"

//配置项结构体的声明
typedef struct {
	char ItemName[50];
	char ItemContent[450];
}CConfItem ,*PLCConfItem;

typedef struct{
	int log_level;//日志文件中保存的日志的最高等级
	int fd;//保存日志的文件描述符
}ngx_log_t;

extern char** g_os_argv;//指向命令参数
extern char* gp_envmem;//指向保存环境变量的新内存
extern int g_environlen;//环境变量的长度


extern pid_t ngx_pid;//进程id
extern pid_t ngx_ppid;//进程ppid
extern ngx_log_t ngx_log;//日志结构

extern int g_argvmemneed;//参数的长度
extern int g_envmemneed;//环境变量的长度
extern int g_os_argc;//命令参数的个数
extern int ngx_process; //表示是master worker进程
extern sig_atomic_t ngx_reap;//记录子进程结束
extern int g_daemonized;//是否开启守护进程

extern CSocket g_socket;//epoll模型对象

#endif
