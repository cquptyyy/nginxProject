
#include "ngx_func.h"
#include "ngx_c_conf.h"
#include "ngx_c_socket.h"
#include "ngx_c_conf.h"
#include "ngx_macro.h"
#include <sys/types.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>


CSocket::CSocket(){
  epollfd=-1;
  listenPortCount=1;
  workerConnectionMaxNum=1;
  pconnection=nullptr;
  pfreeConnection=nullptr;
}


CSocket::~CSocket(){
  ngx_log_stderr(0,"CSocket::~CSocket()");
  std::vector<lpngx_listening_t>::iterator pos;
  for(pos=listenSocketList.begin();pos!=listenSocketList.end();++pos){
    delete (*pos);
    (*pos)=nullptr;
  }
  listenSocketList.clear();
}









//初始化--执行开启监听套接字的函数
bool CSocket::Initialize(){
  read_conf();
  return ngx_open_listening_sockets();
}

//读取连接配置项
void CSocket::read_conf(){
  CConfig* pconfig=CConfig::GetInstance();
  workerConnectionMaxNum=pconfig->GetIntDefault("worker_connections",workerConnectionMaxNum);
  listenPortCount=pconfig->GetIntDefault("listenPortCount",listenPortCount);
}















//开启监听套接字的函数
//1.从配置文件中找到需要开启的监听套接字的个数
//2.调用socket创建监听套接字
//3.调用setsockopt启动监听套接字的地址复用 防止服务器崩溃 监听套接字处于TIME_WAIT状态 不能立即重启
//4.调用setnonblocking将监听套接字设置为非阻塞 没有连接到来就 轮询
//5.调用bind将监听套接字绑定需要监听的ip地址和port端口号
//6.调用listen开启监听套接字的监听功能
//7.将监听套接字和监听的端口加入listenSocketList监听套接字的数组中去
bool CSocket::ngx_open_listening_sockets(){
  struct sockaddr_in server_addr;
  memset(&server_addr,0,sizeof(server_addr));
  server_addr.sin_family=AF_INET;
  server_addr.sin_addr.s_addr=htonl(INADDR_ANY);

  CConfig* pconfig=CConfig::GetInstance();
  for(int i=0;i<listenPortCount;++i){
    int listenfd=socket(AF_INET,SOCK_STREAM,0); ;
    if(listenfd==-1){
      ngx_log_stderr(errno,"CSocket::Initialize()中的socket() failed,i=%d",i);
      close(listenfd);
      return false;
    }

    int reuseaddr=1;
    if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,(const void*)&reuseaddr,sizeof(reuseaddr))==-1){
      ngx_log_stderr(errno,"CSocket::Initialize()中的setsockopt() failed i=%d",i);
      close(listenfd);
      return false;
    }

    if(setnonblocking(listenfd)==-1){
      ngx_log_stderr(errno,"CSocket::Initialize()中的setnonblocking() failed i=%d",i);
      close(listenfd);
      return false;
    }

    char strport[40]={0};
    snprintf(strport, sizeof(strport), "ListenPort%d", i);

    int port=pconfig->GetIntDefault(strport,10000);
    server_addr.sin_port=htons((uint16_t)port);

    if(bind(listenfd,(const struct sockaddr*)&server_addr,sizeof(server_addr))==-1){
      ngx_log_stderr(errno,"CSocket::Initialize()中的bind() failed i=%d",i);
      close(listenfd);
      return false;
    }

    if(listen(listenfd,NGX_LISTEN_BACKLOG)==-1){
      ngx_log_stderr(errno,"CSocket::Initialize()中的listen() failed i=%d",i);
      close(listenfd);
      return false;
    }

    lpngx_listening_t listensockitem=new ngx_listening_t;
    memset(listensockitem,0,sizeof(ngx_listening_t));
    listensockitem->fd=listenfd;
    listensockitem->port=port;
    listenSocketList.push_back(listensockitem);
    ngx_log_error_core(NGX_LOG_INFO,0,"监听%d端口成功",port);
  }
  return true;
}










//将设置监听套接字设置为非阻塞
//使用ioctl或者fcntl
bool CSocket::setnonblocking(int fd){
  //方法一
  int set=1;
  if(ioctl(fd,FIONBIO,&set)==-1){
    ngx_log_error_core(NGX_LOG_ALERT,errno,"setnonblocking()中的ioctl()failed");
    return false;
  }
  return true;

  //方法二
  // int opts=fcntl(fd,F_GETFL);
  // if(opts<0){
  //   ngx_log_error_core(NGX_LOG_ALERT,errno,"setnonblocking()中的fcntl()failed");
  //   return false;
  // }
  // opts|=O_NONBLOCK;
  // if(fcntl(fd,F_SETFL,opts)<0){
  //   ngx_log_error_core(NGX_LOG_ALERT,errno,"setnonblocking()中的fcntl()failed");
  //   return false;
  // }
  // return true;
}




//关闭监听套接字及端口
void CSocket::ngx_close_listening_sockets(){
  for(int i=0;i<listenPortCount;++i){
    close(listenSocketList[i]->fd);
    ngx_log_error_core(NGX_LOG_INFO,0,"关闭监听%d端口",listenSocketList[i]->port);
  }
}














//epoll模型初始化
//1.调用epoll_create获取epoll对象
//2.开辟存储连接connection的内存
//3.初始化内存中的连接,初始化空闲连接链表
//4.创建所有监听套接字的连接，将这些连接加入epoll对象中进行监听
int CSocket::ngx_epoll_init(){
  epollfd=epoll_create(workerConnectionMaxNum);
  if(epollfd==-1){
    ngx_log_stderr(errno,"CSocket::ngx_epoll_init()中的epoll_create failed");
    exit(2);
  }

  connectionNum=workerConnectionMaxNum;
  pconnection=new ngx_connection_t[connectionNum];
  // for(int i=0;i<connectionNum;++i){
  //   pconnection[i].instance=1;
  // }

  lpngx_connection_t nex=nullptr;
  for(int i=connectionNum-1;i>=0;--i){
    pconnection[i].next=nex;
    pconnection[i].fd=-1;
    pconnection[i].instance=1;
    pconnection[i].sequence=0;
    nex=&pconnection[i];
  }
  pfreeConnection=nex;
  freeConnetionNum=connectionNum;

  std::vector<lpngx_listening_t>::iterator pos;
  for(pos=listenSocketList.begin();pos!=listenSocketList.end();pos++){
    lpngx_connection_t listencon=ngx_get_connection((*pos)->fd);
    if(listencon==nullptr){
      ngx_log_stderr(errno,"CSocket::ngx_epoll_init()中的ngx_get_connection() failed");
      exit(2);
    }
    listencon->fd=(*pos)->fd;
    (*pos)->connection=listencon;
    listencon->rhandler=&CSocket::ngx_event_accept;

    if(ngx_epoll_add_event((*pos)->fd,1,0,0,EPOLL_CTL_ADD,listencon)==-1){
      exit(2);
    }

  }
  return 1;
}












//将连接加入epoll对象进行监听
//1.将连接需要监听的事件加入epoll_event中
//2.将instance和连接connection放入epoll_event中
//3.将文件描述符及监听的事件加入epoll对象中
int CSocket::ngx_epoll_add_event(int fd,int revent,int wevent,
  uint32_t otherflag,uint32_t eventtype,lpngx_connection_t conn){
  struct epoll_event ev;
  memset(&ev,0,sizeof(ev));
  if(revent==1){
    ev.events=EPOLLIN|EPOLLRDHUP;
  }
  else{

  }
  if(otherflag!=1){
    ev.events|=otherflag;
  }
  ev.data.ptr=(void*)((uint64_t)conn|conn->instance);

  if(epoll_ctl(epollfd,eventtype,fd,&ev)==-1){
    ngx_log_stderr(errno,"CSocket::ngx_epoll_add_event()中的epoll_ctl() failed");
    return -1;
  }
  return 1;
}











//等待事件就绪,对就绪事件进行处理
//1.通过epoll_wait获取就绪事件的个数,若事件的个数为-1,判断错误码打印日志
//2.循环找出就绪事件，获取就绪事件的连接 instance 就绪事件的类型 
//3.根据instance 和 事件的文件描述符判断连接是否有效
//4.若连接有效则调用响应的事件处理函数
int  CSocket::ngx_epoll_process_event(int time){
  int n=epoll_wait(epollfd,events,NGX_MAX_EVENTS,time);
  if(n==-1){
    if(errno==EINTR){//收到信号，导致系统调用出错,正常
      ngx_log_error_core(NGX_LOG_INFO,errno,"CSocket::ngx_epoll_process_event()中的epoll_wait() failed");
      return 1;
    }
    else{
      ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocket::ngx_epoll_process_event()中的epoll_wait() failed");
      return 0;
    }
  }
  if(n==0){
    if(time!=-1){//没有事件就绪，超时返回很正常
      return 1;
    }
    ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocket::ngx_epoll_process_event()中的epoll_wait() 设置了无限等待时间，返回却没有事件");
    return 0;
  }

 
  for(int i=0;i<n;++i){
    struct epoll_event ev=events[i];
    uint64_t instance=(uint64_t)ev.data.ptr&1;
    lpngx_connection_t conn=(lpngx_connection_t)((uint64_t)ev.data.ptr&(~1));

    if(conn->fd==-1){
      //处理过期连接
      //连接已关闭 一次性接收三个事件 事件一和事件三属于同一文件描述的同一个连接 
      //处理事件一时由于业务需要，已经将连接关闭，文件描述符被置为-1
      ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocket::ngx_epoll_process_event()中遇到过期的连接事件：%P",conn);
      continue;
    }
    if(instance!=conn->instance){
      //一次性收到三个事件，事件一和事件三属于同一个文件描述符的不同事件，事件一由于业务需要将文件描述符关闭
      //事件二是获取新连接，巧合的是事件一释放的连接被分配给了处理事件二的新连接
      //事件三此时的conn是事件二的新连接，但是在ev.data.ptr中的instance却没有改变 依旧是事件一时的 每个连接的epoll_event不同
      //事件二中的conn->instance不同和事件一连接的instance不同  应为ngx_get_connection会将连接中的instance置反
      //通过判断instance过滤掉过期的连接
      ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocket::ngx_epoll_process_event()中遇到过期的连接事件：%P",conn);
    }

    uint32_t revents=ev.events; 
    if(revents&(EPOLLERR|EPOLLHUP)){
      revents|=(EPOLLIN|EPOLLOUT);
    }
    if(revents&EPOLLIN){
      (this->*(conn->rhandler))(conn);
    }
    if(revents&EPOLLOUT){

    }
  }
  return 1;
}
