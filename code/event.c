#include "core.h"
#include "os.h"
#include "allocator.h"
#include "event.h"

#include <errno.h>

#ifndef INET6_ADDRSTRLEN 
#define INET6_ADDRSTRLEN 65
#endif 

priv_func
ServerTcpHandle
tcpServerHandle(ArenaAllocator *a, EventOptions *opt)
{
ServerTcpHandle tcp       = {0};

    tcp.server            = osSocketGetServerSocket(opt->server_ip, opt->server_port);
    tcp.multiplexer       = osGetMultiplexerHandle(a, opt->multiplexer_type, opt->event_count);

    osAddSocketToMultiplexer(tcp.multiplexer, tcp.server, FlagRead);
    tcp.optional_timeout  = opt->timeout;
    return tcp;
}

priv_func
EventQueueList
allocEventQueues(ArenaAllocator *a, s32 event_count)
{
EventQueueList queues = {0};
Event *idle, *waiting, *ready;
  
    idle      = 0;
    waiting   = 0;
    ready     = 0;

    idle      = (Event*)push_arena_array(a, Event, event_count);
    waiting   = (Event*)push_arena_array(a, Event, event_count);
    ready     = (Event*)push_arena_array(a, Event, event_count); 

    queues.q_idle.q             = idle;
    queues.q_idle.capacity      = event_count;

    queues.q_waiting.q          = waiting;
    queues.q_waiting.capacity   = event_count;

    queues.q_ready.q            = ready;  
    queues.q_waiting.capacity   = event_count;

    return queues;
}

EventHandle 
eventInitHandle(ArenaAllocator *a, EventOptions opt)
{
EventHandle ret   = {0};
EventQueueList  queues;

    if(opt.event_type == EventTcpServer) {
        ret.server_tcp = tcpServerHandle(a, &opt); 
    }
    ret.options = opt; 

    queues = allocEventQueues(a, opt.event_count);

    ret.queues   = queues;
    return ret;
}

enum EventResult
eventQueueRunLoop(EventHandle h)
{
s32  events_now, event_type, event_result;
handle_event_func       *callback;
handle_event_error_func *error_callback;
s32 ret;
Event e;
EventOptions opt;
EpollHandle  epoll_handle;
ArenaAllocator *event_allocation;
ScrachAllocator  scrach_event;

  event_type      = GetEventHandleType(h);
  callback        = GetEventCallback(h);
  error_callback  = GetEventErrorCallback(h);
  opt             = GetEventHandleOptions(h);

#if 0 // If there is more stuff to do implement this
  if(event_type == EventTcpServer) {

  }
#endif
  
  ServerTcpHandle tcp;
  tcp = h.server_tcp;
  epoll_handle = tcp.multiplexer.e;
  event_allocation = arenaNew();
  scrach_event = scrachNew(event_allocation);

  for(; ; scrachEnd(scrach_event)) {
    events_now = osMultiplexerPoll(tcp.multiplexer, tcp.optional_timeout);
    if(events_now == -1) {
      if(errno == EINTR) 
        continue;
      Event error = {0};
      error.err = errno;
      error_callback(error, opt.user_defined_ptr);
      return EventError;
    }
    if((opt.sleep_sec > 0 || opt.sleep_nsec > 0) 
        && events_now == 0) 
    {
      osSleep(opt.sleep_sec, opt.sleep_nsec);
      continue;
    }

    // This is something that is always gonna happen
    for(int n = 0; n < events_now; n++) {
        if(epoll_handle.events[n].data.fd == ST_TO_DATA(tcp.server)) {
            SocketType client = osSocketAcceptConnection(tcp.server);
            if(ST_TO_DATA(client) == -1) {
                  if(errno == EWOULDBLOCK ||
                     errno == EAGAIN ||
                     errno == EINTR)
                    continue;
                  else {
                      Event error = {0};
                      error.err = errno;
                      error_callback(error, opt.user_defined_ptr);
                      return EventError;
                  }
            }
            if(!osAddSocketToMultiplexer(tcp.multiplexer, client, FlagRead | FlagEdgeTrigger | FlagWrite)) {
                Event error = {0};
                error.err   = errno;
                char ip_str_buf[INET6_ADDRSTRLEN];               
                Str  ip_buf = cstr_static(ip_str_buf);
                error.info.client_ip = ip_buf;
                osGetSocketConnectionInfo(client, ip_buf, &error.info.port);
                error_callback(error, opt.user_defined_ptr);
                return EventError;
            } 
        } else {
              Event e = {0}; 
              SocketType client = DATA_TO_ST(epoll_handle.events[n].data.fd);
              e.client = client;
              char ip_str_buf[INET6_ADDRSTRLEN];               
              Str  ip_buf = cstr_static(ip_str_buf);
              e.info.client_ip = ip_buf;
              osGetSocketConnectionInfo(client, ip_buf, &e.info.port);
              e.ev_buff = strbuf_alloc(.flags=StrBufIncrease|StrBufSharedArena,.shared_arena=event_allocation);
              if(osFullReadFromSocket(e.client, &e.ev_buff) != 0) {
                  ret = CallbackCloseClient | CallbackError;
              } else {
                  ret = callback(e, opt.user_defined_ptr);
              }
              if(ret & CallbackError) {
                e.err = errno;
                error_callback(e, opt.user_defined_ptr);
              }

              osRemoveSocketFromMultiplexer(tcp.multiplexer, client);
              osCloseSocket(client);
        }
    }
  } 

  return 0;
}
