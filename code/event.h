#pragma once

#include "core.h"
#include "allocator.h"
#include "str.h"
#include "str_buf.h"
#include "os.h"

#if __cplusplus
extern "C" {
#endif

typedef struct Event Event;
typedef struct ClientInfo ClientInfo;
typedef struct EventQueue EventQueue;
typedef struct EventQueueList EventQueueList;
typedef struct ServerTcpHandle ServerTcpHandle;
typedef struct EventOptions EventOptions;

#define handle_event_func_def(name) enum CallbackResult name(Event event, void *user_defined_ptr)
typedef handle_event_func_def(handle_event_func);

#define handle_event_error_func_def(name) void name(Event event, void *user_defined_ptr)
typedef handle_event_error_func_def(handle_event_error_func);

enum EventResult 
{
  EventHandled      = (1 << 0),
  EventInterruped   = (1 << 1),
  EventBlocked      = (1 << 2),
  EventNothingToDo  = (1 << 3),
  EventError        = (1 << 4)
};

enum EventHandleType
{
  EventTcpServer = 0
};

enum CallbackResult 
{
  CallbackSuccess       = (1 << 0),
  CallbackError         = (1 << 1),
  CallbackCloseClient   = (1 << 2),
  CallbackKeepClient    = (1 << 3)
};

struct ClientInfo 
{
  Str client_ip;
  s32 port;
};

struct Event 
{
  Event         *next;
  Event         *prev;

  SystemError   err;
  StrBuf        ev_buff;
  ClientInfo    info;
  SocketType    client;
};

struct EventQueue {
  Event *q;
  s32   count;
  s32   capacity;
};

struct EventQueueList
{
   EventQueue    q_idle;
   EventQueue    q_waiting;
   EventQueue    q_ready;
};

struct ServerTcpHandle
{
  SocketType        server;
  MultiplexerHandle multiplexer; 
  s32               optional_timeout;
};

struct EventOptions 
{
    Str server_ip;
    Str server_port;
    void *user_defined_ptr;
    handle_event_func *callback;
    handle_event_error_func *callback_error;
    s32 event_type;
    s32 event_count;
    s32 multiplexer_type;
    s32 timeout;
    s32 sleep_sec;
    s32 sleep_nsec;
};

typedef struct EventHandle EventHandle;
struct EventHandle 
{
  union {
    ServerTcpHandle server_tcp;
  }; 

  EventQueueList  queues;
  EventOptions    options;
};

#define GetEventHandleOptions(e) (e).options
#define GetEventHandleType(e)    (e).options.event_type
#define GetEventCallback(e)      (e).options.callback
#define GetEventErrorCallback(e) (e).options.callback_error

enum EventResult eventQueueRunLoop(EventHandle h);
EventHandle eventInitHandle(ArenaAllocator *a, EventOptions opt);

#if __cplusplus
}
#endif

