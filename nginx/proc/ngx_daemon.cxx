#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ngx_global.h"
#include "ngx_func.h"
#include "ngx_macro.h"

//设置守护进程 脱离终端，进程进入后台
//1.创建子进程，子进程继续执行 父进程return
//2.开启新的会话，setsid脱离终端
//3.umask设置为0，防止影响设置文件权限
//4.将标准输入输出重定位到/dev/null

int ngx_daemon(){
  pid_t pid=fork();
  if(pid==-1){
    ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中的fork failed");
    return -1;
  }
  if(pid>0)return 1;

  ngx_ppid=ngx_pid;
  ngx_pid=getpid();
  if(setsid()==-1){
    ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中的setsid failed");
    return -1;
  }

  umask(0);

  int fd=open("/dev/null",O_RDWR);
  if(fd==-1){
    ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemo()中open() failed");
    return -1;
  }
  if(dup2(fd,STDIN_FILENO)==-1){
    ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemo()中dup2(STDIN) failed");
    return -1;
  }
  if(dup2(fd,STDOUT_FILENO)==-1){
    ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemo()中dup2(STDOUT) failed");
    return -1;
  }

  if(fd>STDERR_FILENO){
    if(close(fd)==-1){
      ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中的close() failed");
      return -1;
    }
  }
  return 0;
}