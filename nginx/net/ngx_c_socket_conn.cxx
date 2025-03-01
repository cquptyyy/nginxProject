#include "ngx_global.h"
#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_c_socket.h"




//获取连接 会将空闲连接清空 将原来的!instance赋值给instance
lpngx_connection_t CSocket::ngx_get_connection(int fd){
  lpngx_connection_t conn=pfreeConnection;
  if(conn==nullptr){
    ngx_log_error_core(NGX_LOG_ALERT,0,"CSocket::ngx_get_connection()中获取空闲连接时空闲连接的链表是空的!");
    return nullptr;
  }
  pfreeConnection=conn->next;
  freeConnetionNum--;

  uint64_t instance=conn->instance;
  uint64_t sequence=conn->sequence;

  memset(conn,0,sizeof(ngx_connection_t));
  conn->fd=fd;

  conn->instance=!instance;
  conn->sequence=sequence;
  ++conn->sequence;
  return conn;
}




//将空闲连接归还到空闲连接池
void CSocket::ngx_free_connection(lpngx_connection_t conn){
  conn->next=pfreeConnection;
  freeConnetionNum++;
  pfreeConnection=conn;
  conn->sequence++;

}




