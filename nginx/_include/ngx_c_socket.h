#ifndef __NGX_SOCKET_H__
#define __NGX_SOCKET_H__

#define NGX_LISTEN_BACKLOG 511
#define NGX_MAX_EVENTS 512

#include <vector>
#include <sys/epoll.h>
#include <sys/socket.h>

typedef class CSocket CSocket;
typedef struct ngx_listening_s ngx_listening_t,*lpngx_listening_t;
typedef struct ngx_connection_s ngx_connection_t,*lpngx_connection_t;
typedef void (CSocket::*ngx_event_handler_t)(lpngx_connection_t);



//监听套接字结构体
struct ngx_listening_s{
  int port;//监听端口
  int fd;//监听套接字
  lpngx_connection_t connection; //监听套接字对应的连接结构
};



//连接结构体
struct ngx_connection_s{
  int fd;//连接文件描述符
  lpngx_listening_t listening;//连接对应的监听套接字

  unsigned  instance:1;//保证过滤过期连接
  uint64_t  sequence;//连接序号
  struct sockaddr addr;//存储连接的地址

  int wready;//写准备

  ngx_event_handler_t rhandler;//读事件处理函数
  ngx_event_handler_t whandler;//写事件处理函数

  lpngx_connection_t next;//指向下一个连接
};



//套接字结构  是epoll结构
class CSocket{
public:
  CSocket();
  ~CSocket();
public:
  virtual bool Initialize();
private:
  bool ngx_open_listening_sockets();//开启监听套接字
  void ngx_close_listening_sockets();//关闭监听套接字
  bool setnonblocking(int fd);//将文件描述符设置为非阻塞

public:
  int ngx_epoll_init();//epoll初始化 创建epoll
  int ngx_epoll_add_event(int fd,int revent,int wevent,uint32_t otherflag,uint32_t eventtype,lpngx_connection_t conn);//添加事件进epoll对象中
  int ngx_epoll_process_event(int time);//等待事件就绪

private:
  void read_conf();
  void ngx_event_accept(lpngx_connection_t listenConn);//接收新连接
  void ngx_close_accept_connection(lpngx_connection_t conn);//关闭

  lpngx_connection_t ngx_get_connection(int fd); //获取连接
  void ngx_free_connection(lpngx_connection_t conn);//释放连接
  void ngx_wait_request_handler(lpngx_connection_t conn);//连接读时间
  size_t ngx_sock_ntop(struct sockaddr* addr,int port,char* text,size_t len);//获取对端信息

private:
  std::vector<lpngx_listening_t> listenSocketList;//监听连接的数组
  int listenPortCount;//监听的端口数量
  int workerConnectionMaxNum;//支持的最大连接数量
  int epollfd;//epoll对象的文件描述符

  lpngx_connection_t pconnection;//连接链表
  int connectionNum;
  lpngx_connection_t pfreeConnection;//空闲连接链表
  int freeConnetionNum;

  struct epoll_event events[NGX_MAX_EVENTS];//用于接收就绪事件
};

#endif