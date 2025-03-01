

#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ngx_macro.h"
#include "ngx_func.h"
#include "ngx_global.h"
typedef struct{
  int signo;
  const char* signame;
  void (*handler)(int signo,siginfo_t* info,void* ucontext);
}ngx_signal_t; 


static void ngx_signal_handler(int signo,siginfo_t* info,void* ucontext);
static void ngx_process_get_status();

ngx_signal_t signals[]={
  {SIGINT,"SIGINT",ngx_signal_handler},
  {SIGHUP,"SIGHUP",ngx_signal_handler},
  {SIGQUIT,"SIGQUIT",ngx_signal_handler},
  {SIGTERM,"SIGTERM",ngx_signal_handler},
  {SIGCHLD,"SIGCHLD",ngx_signal_handler},
  {SIGIO,"SIGIO",ngx_signal_handler},
  {SIGALRM,"SIGALRM",ngx_signal_handler},
  {SIGUSR1,"SIGUSER1",ngx_signal_handler},
  {SIGUSR2,"SIGUSER2",ngx_signal_handler},
  {SIGWINCH,"SIGWINCH",ngx_signal_handler},
  {SIGSYS,"SIGSYS",ngx_signal_handler},
  {0,nullptr,nullptr},
};

int ngx_signal_init(){
  ngx_signal_t* sig;
  struct sigaction sa;
  memset(&sa,0,sizeof(sa));

  for(sig=signals;sig->signo!=0;sig++){
    if(sig->handler!=nullptr){
      sa.sa_sigaction=sig->handler;
      sa.sa_flags=SA_SIGINFO;
    }
    else{sa.sa_handler=SIG_IGN;}

    sigemptyset(&sa.sa_mask);

    if(sigaction(sig->signo,&sa,nullptr)==-1)
    {ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_signal_init()中的sigaction(%s) failed",sig->signame);}
    else{ngx_log_stderr(0,"sigaction(%s) success! ",sig->signame);}
  }
  return 0;
}

//信号处理函数
//1.找到信号所属的ngx_signal_t
//2.判断是哪个进程收到信号(master worker other)
//3.打印收到信号的日志(进程发出的信号  非进程发出的信号)
//4.如果是子进程退出发出的信号 获取子进程的状态避免  僵尸进程
static void ngx_signal_handler(int signo,siginfo_t* info,void* ucontext){
  ngx_signal_t* sig;
  for(sig=signals;sig->signo!=0;++sig){
    if(sig->signo==signo){
      break;
    }
  }

  if(ngx_process==NGX_PROCESS_MASTER){
    if(signo==SIGCHLD){
      ngx_reap=1;
    }
  }
  else if(ngx_process==NGX_PROCESS_WORKER){

  }
  else{

  }

  const char* action="";
  if(info&&info->si_pid){
    ngx_log_error_core(NGX_LOG_NOTICE,0,"signal %d (%s) receive from %P%s",signo,sig->signame,info->si_pid,action);
  }
  else{
    ngx_log_error_core(NGX_LOG_NOTICE,0,"signal %d (%s) receive  %s",signo,sig->signame,action);
  }

  if(signo==SIGCHLD){
    ngx_process_get_status();
  }
}

//获取子进程结束的状态  waitpid 防止子进程成为僵尸进程
//1.waitpid 第一个参数为 -1表示接收任意信号  第二个参数 status获取子进程结束状态
// 第三个参数  WNOHANG表示非阻塞
//2.waitpid返回值>0表示调用成功 =0子进程没有结束 =-1表示调用失败
//3.waitpid调用成功,判断子进程是不是因为信号而结束 信号结束WTERMSIG获取信号 
//不是信号结束获取子进程传给exit或者_exit的值
static void ngx_process_get_status(){
  pid_t pid;
  int  status;
  int one=0;//信号是否被正常处理过一次
  while(1){
    pid=waitpid(-1,&status,WNOHANG);
    if(pid==0){return;}
    else if(pid==-1){
      int err=errno;
      //信号终端调用waitpid
      if(err==EINTR){continue;}
      //信号已被处理过
      else if(err==ECHILD&&one){return;}
      //子进程不存在
      else if(err==ECHILD){
        ngx_log_error_core(NGX_LOG_INFO,err,"waitpid() failed!");
        return;
      }
      ngx_log_error_core(NGX_LOG_ALERT,err,"waitpid()  failed!");
      return;
      //
    }

    one=1;
    //waitpid调用成功
    if(WTERMSIG(status)){//子进程收到信号结束
      ngx_log_error_core(NGX_LOG_ALERT,0,"pid = %P exited on signal %d!",pid,WTERMSIG(status)); 
    }
    else{
      ngx_log_error_core(NGX_LOG_ALERT,0,"pid = %P exited with code %d!",pid,WEXITSTATUS(status));
    }
    return;
  }
}