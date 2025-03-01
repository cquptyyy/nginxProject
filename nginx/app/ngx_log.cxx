#include <string.h>
#include <stdarg.h>//va_start va_end
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/time.h>
#include "ngx_macro.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_c_conf.h"


ngx_log_t ngx_log;

static char ngx_log_levels[][20]={
  "stderr",
  "emerg",
  "alert",
  "crit",
  "error",
  "warn",
  "notice",
  "info",
  "debug",
};



//错误日志打印函数 例如：ngx_log_stderr(0,"this my log have err file=%s not open","nginx.conf");
void ngx_log_stderr(int err,const char* fmt,...){
  char errstr[NGX_MAX_LOG_STR+1];
  char* last=errstr+NGX_MAX_LOG_STR;
  memset(errstr,0,sizeof(errstr));
  char* p=ngx_memcpy(errstr,"nginx: ",7);

  va_list args;
  va_start(args,fmt);
  p=ngx_vslprintf(p,last,fmt,args);
  va_end(args);

  if(err){p=ngx_log_errno(p,last,err);}

  if(p>=last){p=last-1;}
  *p++='\n';
  write(STDERR_FILENO,errstr,p-errstr);

  if(ngx_log.fd>STDERR_FILENO){
    ngx_log_error_core(NGX_LOG_STDERR,err,errstr);
  }
}




//日志初始化
void ngx_log_init(){
  CConfig* pconfig=CConfig::GetInstance();
  const char* plogname=pconfig->GetString("log");
  if(plogname==nullptr){plogname=NGX_ERROR_LOG_PATH;}
  ngx_log.log_level=pconfig->GetIntDefault("loglevel",NGX_LOG_NOTICE);

  ngx_log.fd=open(plogname,O_WRONLY|O_APPEND|O_CREAT,0644);
  if(ngx_log.fd==-1){
    ngx_log.fd=STDERR_FILENO;
    ngx_log_stderr(errno,"could not open log file: open \"%s\" failed",plogname);
  }
}



//错误码消息打印
char* ngx_log_errno(char* buf,char*last,int err ){
  char* perrorinfo=strerror(err);
  size_t len=strlen(perrorinfo);
  
  char leftstr[10];
  sprintf(leftstr,"(%d",err);
  size_t leftlen=strlen(leftstr);
  const char* rightstr=")";
  size_t rightlen=strlen(rightstr);
  size_t extralen=leftlen+rightlen;

  if(buf+extralen+len<last){
    buf=ngx_memcpy(buf,leftstr,leftlen);
    buf=ngx_memcpy(buf,perrorinfo,len);
    buf=ngx_memcpy(buf,rightstr,rightlen);
  }
  return buf;
}


//日志打印包含 日志等级 错误码信息
void ngx_log_error_core(int level,int err,const char* fmt,...){
  if(level>ngx_log.log_level)return;
  char errstr[NGX_MAX_LOG_STR+1];
  char* last=errstr+NGX_MAX_LOG_STR;
  memset(errstr,0,sizeof(errstr));

  struct timeval tv;
  struct tm tm;
  time_t sec;

  gettimeofday(&tv,nullptr);
  sec=tv.tv_sec;
  localtime_r(&sec,&tm);
  tm.tm_year+=1900;
  tm.tm_mon++;

  char strcurrtime[40]={0};
  ngx_slprintf(strcurrtime,strcurrtime+sizeof(strcurrtime)-1,
  "%4d/%02d/%02d %02d:%02d:%02d",
  tm.tm_year,tm.tm_mon,tm.tm_mday,
  tm.tm_hour,tm.tm_min,tm.tm_sec
  );

  char* p=ngx_memcpy(errstr,strcurrtime,strlen(strcurrtime));
  p=ngx_slprintf(p,last," [%s] %P:",ngx_log_levels[level],ngx_pid);

  va_list args;
  va_start(args,fmt);
  p=ngx_vslprintf(p,last,fmt,args);
  va_end(args);

  if(p>=last){p=last-1;}
  *p++='\n';
  if(err!=0){ngx_log_errno(p,last,err);}
  ssize_t n=write(ngx_log.fd,errstr,p-errstr);
  if(n==-1){
    //磁盘空间不足
    if(errno==ENOSPC){

    }
    else if(ngx_log.fd!=STDERR_FILENO){
      write(STDERR_FILENO,errstr,p-errstr);
    }
  }
}