#include "ngx_macro.h"
#include "ngx_func.h"
#include "ngx_global.h"
#include "ngx_c_socket.h"
#include <errno.h>

void CSocket::ngx_event_accept(lpngx_connection_t listenconn){
  int use_accept4=1;
  int connfd;
  struct sockaddr addr;
  socklen_t len=sizeof(addr);
  while(1){
    if(use_accept4==1){
      connfd=accept4(listenconn->fd,&addr,&len,SOCK_NONBLOCK);
    }
    else{
      connfd=accept(listenconn->fd,&addr,&len);
    }
    if(connfd==-1){
      int err=errno;
      if(err==EAGAIN||err==EWOULDBLOCK){//accept未收到连接，期待将accept设置为非阻塞或者再来一次accept
        return ;
      }
      int level=NGX_LOG_ALERT;
      if(err==ECONNABORTED){//客户端发送RST报文将位于全连接队列中的连接断开进行重新连接 致使accept不能获取全连接中的连接
        level=NGX_LOG_ERROR;
      }
      if(err==EMFILE||err==ENFILE){//文件描述符不够用 ulimit -n
        level=NGX_LOG_CRIT;
      }
      if(use_accept4==1&&err==ENOSYS){//没有实现accept4
        use_accept4=0;
        continue;
      }
      ngx_log_error_core(level,err,"CSocket::ngx_event_accept()中的accept() failed");
    }
    lpngx_connection_t conn=ngx_get_connection(connfd);
    if(conn==nullptr){
      if(close(connfd)==-1){
        ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocket::ngx_event_accept()中的close() failed");
        return;
      }
    }
    conn->listening=listenconn->listening;
    memcpy(&conn->addr,&addr,len);
    conn->wready=1;
    conn->rhandler=&CSocket::ngx_wait_request_handler;
    if(ngx_epoll_add_event(connfd,1,0,EPOLLET,EPOLL_CTL_ADD,conn)==-1){
      ngx_close_accept_connection(conn);
      return;
    }
    break;
  }
}


void CSocket::ngx_close_accept_connection(lpngx_connection_t conn){//关闭新连接
  int fd=conn->fd;
  conn->fd=-1;
  ngx_free_connection(conn);
  if(close(fd)==-1){
    ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocket::ngx_close_accept_connection()中的close() failed");
  }
}