#include "ngx_global.h"
#include "ngx_func.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>


//将环境变量保存到新的内存中，防止新的进程名将环境变量覆盖
void ngx_init_setproctitle(){
  for(int i=0;environ[i];++i){
    g_environlen+=(strlen(environ[i])+1);
  }
  //开辟保存环境变量的新内存
  gp_envmem=new char[g_environlen];
  char* ptmp=gp_envmem;
  for(int i=0;environ[i];++i){
    size_t len=strlen(environ[i])+1;
    strcpy(ptmp,environ[i]);
    environ[i]=ptmp;
    ptmp+=len;
  }
}


//设置程序进程的名字
void ngx_setproctitle(const char* title){
  size_t len=strlen(title);
  size_t e_environlen=0;
  for(int i=0;g_os_argv[i];++i){
    e_environlen+=(strlen(g_os_argv[i])+1);
  }
  size_t esy=g_environlen+e_environlen;
  if(esy<len)return;
  g_os_argv[1]=nullptr;
  strcpy(g_os_argv[0],title);
  char* ptmp=g_os_argv[0];
  ptmp+=len;
  size_t cha=esy-len;
  memset(ptmp,0,cha);
}


