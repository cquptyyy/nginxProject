#include "ngx_global.h"
#include "ngx_c_socket.h"

//调用epoll_wait等待事件就绪并处理
void ngx_process_events_and_timers(){//无限等待
  g_socket.ngx_epoll_process_event(-1);
}