#ifndef __NGX_FUNC_H__
#define __NGX_FUNC_H__
#include <stdarg.h>

//将字符串左右的空格去掉
void Ltrim(char* string);
void Rtrim(char* string);

//为环境变量开辟新内存
void ngx_init_setproctitle();

//设置进程名
void ngx_setproctitle(const char*);

void ngx_log_stderr(int ,const char*,...);//打印错误日志
char* ngx_vslprintf(char*,char*,const char*,va_list);//格式化打印日志
char* ngx_log_errno(char*,char*,int);//打印错误码消息
void ngx_log_init();//初始化日志
char* ngx_slprintf(char*,char* ,const char*,...);//打印日志
void ngx_log_error_core(int,int ,const char* ,...);//带日志等级和错误码的日志打印

int ngx_signal_init();//信号初始化 注册信号的处理函数
void ngx_master_process_cycle();//主进程函数

int ngx_daemon();//创建守护进程
void ngx_process_events_and_timers();//等待事件就绪

#endif

