#ifndef __NGX_MACRO_H__
#define __NGX_MACRO_H__

#include <string.h>

#define NGX_MAX_LOG_STR 2048 //错误日志存储的最大长度
#define  NGX_INT64_LEN strlen("-9223372036854775808")
#define ngx_memcpy(dest,src,n) (((char*)memcpy(dest,src,n))+n)

#define NGX_LOG_STDERR 0 //控制台错误
#define NGX_LOG_EMERG  1 //紧急错误
#define NGX_LOG_ALERT  2 //警戒错误
#define NGX_LOG_CRIT   3 //严重错误
#define NGX_LOG_ERROR  4 //错误
#define NGX_LOG_WARN   5 //警告
#define NGX_LOG_NOTICE 6 //注意
#define NGX_LOG_INFO   7 //信息
#define NGX_LOG_DEBUG  8 //调试

#define NGX_ERROR_LOG_PATH "logs/error1.log"

#define NGX_PROCESS_MASTER 0
#define NGX_PROCESS_WORKER 1


#endif