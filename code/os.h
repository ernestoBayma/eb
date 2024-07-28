#pragma once

#include "core.h"
#include "allocator.h"
#include "str.h"
#include "str_buf.h"
#include "timer.h"

enum MultiplexerFlags 
{
  FlagRead        = (1 << 0),
  FlagWrite       = (1 << 1),
  FlagEdgeTrigger = (1 << 2)
};

enum FileAction 
{
  ReadAction    = (1 << 0),
  WriteAction   = (1 << 1),
  CreateAction  = (1 << 2),
  DeleteAction  = (1 << 3),
  FolderAction  = (1 << 4),
  AppendAction  = (1 << 5),
  TrucateAction = (1 << 6)
};

enum FileRequestErrors 
{
  FileRequestSuccess            =       0,
  FileRequestNotRegularFile     = (1 << 0),
  FileRequestNotFound           = (1 << 1),
  FileRequestNoPermission       = (1 << 2),
  FileRequestUnkownError        = (1 << 3),
  FileRequestNoReadPermission   = (1 << 4),
  FileRequestNoWritePermission  = (1 << 5),
  FileRequestIsADirectory       = (1 << 6)
};

typedef struct RequestedFile     RequestedFile; 
typedef struct SocketType        SocketType;
typedef struct MultiplexerEvents MultiplexerEvents;
typedef struct FileHandle        FileHandle;
typedef struct ProcessInformation ProcessInformation;

enum MultiplexerHandleType {
  Select=1,
  Epoll,
  Poll
};

typedef struct SelectHandle SelectHandle;
typedef struct EpollHandle  EpollHandle;
typedef struct PollHandle   PollHandle;

#if OS_LINUX
  #include "os_linux.h"
  #include <sys/select.h>
  struct SelectHandle {
     fd_set *readfds;
     fd_set *writefds;
     s32     readfds_count;
     s32     writefds_count;
  };
#if defined(EPOLL_COMPAT_KERNEL) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 3
  #include <sys/epoll.h>
  struct EpollHandle {
      SocketType         epoll_socket;
      struct epoll_event *events;
      s32                 events_count;
  };
#else
  struct EpollHandle {
      u8 invalid;
  };
#endif
  #include <poll.h>
  struct PollHandle {
      struct pollfd *events;
      s32            events_count;
  };

  #define ST_TO_INT(s) (s).socket
  #define ST_TO_DATA(s) (s).socket
  #define INT_TO_ST(i) (SocketType){.socket=(i)}
  #define DATA_TO_ST(d) (SocketType){.socket=(d)}

  #define FH_TO_INT(f) (f).fd
  #define INT_TO_FH(i) (FileHandle){.fd=(i)}
#endif 

typedef struct MultiplexerHandle MultiplexerHandle;
struct MultiplexerHandle {
  union {
    struct SelectHandle s;
    struct EpollHandle  e;
    struct PollHandle   p;
  };
  u8 handle_type;
  bool valid;
};

struct RequestedFile 
{
    FileHandle              handle;
    bool                    exists;
    u32                     size;
    enum FileRequestErrors  errors;
    enum FileAction         valid_actions;
};

struct ProcessInformation 
{
   TimeValue cpu_time_used;
   u64       resident_memory_used;
   u64       page_faults;
   u64       swaps;
};

#if defined(__linux__)
  typedef struct FolderHandleLinux {
    char          path[256];
    RequestedFile folder;
  } FolderHandleLinux;
  #define FolderHandle FolderHandleLinux
  #define SystemError s32
#endif

void        *osMemoryReserve(size_t size);
bool        osMemoryCommit(void *memory, size_t size);
bool        osMemoryDecommit(void *memory, size_t size);
void        osMemoryRelease(void *memory, size_t size);
size_t      osPageSize(void);
RequestedFile osOpenFile(Str filepath, enum FileAction action);
s32         osFullReadFromFile(RequestedFile f, StrBuf *buffer);
s32         osFullWriteToFile(RequestedFile f, Str buffer);
bool        osCloseFile(RequestedFile file);
bool        osCloseSocket(SocketType s);
bool        osSocketSetNonBlocking(SocketType s);
SocketType  osSocketGetServerSocket(Str addr, Str port);
u32         osSendFile(SocketType client, FileHandle file, u32 file_size);
bool        osAddSocketToMultiplexer(MultiplexerHandle multiplexer, SocketType new_socket, enum
MultiplexerFlags flags);
bool        osRemoveSocketFromMultiplexer(MultiplexerHandle multiplexer, SocketType old_socket);
SocketType  osCreateMultiplexerSocket();
bool        osDestroyMultiplexerSocket(SocketType s);
bool        osSocketIsValid(SocketType s);
s32         osFullReadFromSocket(SocketType s, StrBuf *buffer);
s32         osFullWriteToSocket(SocketType s, Str buffer);
bool        osSleep(u32 time_sec, u32 time_nsec);
bool        osAppToBackgroundProcess();
bool        osOpenDirectory(FolderHandle *handle, Str folder_path);
bool        osCloseDirectory(FolderHandle *handle);
RequestedFile osOpenFileFromFolder(FolderHandle *handle, Str filename, enum FileAction action);
bool        osSystemErrorToStr(Str s, SystemError e);
bool        osSetupWorkingDirectory(FolderHandle *work_dir);
bool        osSetSocketOption(SocketType sock, s32 option, void *option_val, size_t option_val_size);
SocketType  osOpenSocket(s32 domain, s32 type, s32 protocol);
bool        osSocketSetCloseOnExec(SocketType s);
SocketType  osSocketAcceptConnection(SocketType server_sock);
MultiplexerHandle osGetMultiplexerHandle(ArenaAllocator *a,s32 type,s32 event_count);
s32         osMultiplexerPoll(MultiplexerHandle h, s32 timeout);
bool        osGetSocketConnectionInfo(SocketType sock, Str ip_buf, s32 *port);
s32         osFullReadFromFile(RequestedFile f, StrBuf *buffer);
ProcessInformation osGetProcessInfo();
TimeValue   osGetTime();
