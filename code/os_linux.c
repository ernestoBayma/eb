#include "core.h"
#include "util.h"
#include "os.h"
#include "allocator.h"
#include "str.h"
#include "str_buf.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/types.h>

void *osMemoryReserve(size_t size)
{
void *memory = 0;
  memory = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return memory;
}

bool osMemoryCommit(void *memory, size_t size)
{
bool ret = false;
    ret = mprotect(memory, size, PROT_READ|PROT_WRITE) == 0;
    return ret;
}

void osMemoryRelease(void *memory, size_t size)
{
    munmap(memory, size);
}

size_t osPageSize(void)
{
      return getpagesize();
}

priv_func 
int fileActionToFlag(enum FileAction action)
{
int flag = 0;

  if((action & WriteAction) && (action & ReadAction))
    flag |= O_RDWR;
  else if((action & ReadAction) && !(action & WriteAction)) 
    flag |= O_RDONLY;
  else if((action & WriteAction) && !(action & ReadAction))
    flag |= O_WRONLY;
  if(action & CreateAction)
    flag |= O_CREAT;
  if(action & AppendAction)
    flag |= O_APPEND;
  if(action & FolderAction)
    flag |= O_DIRECTORY;
  if(action & TrucateAction)
    flag |= O_TRUNC;

  return flag;
}

priv_func
RequestedFile
osOpenFile_impl(FolderHandle *handle, Str filepath, enum FileAction action) 
{
RequestedFile file = {0};
struct stat   st;
int           open_flags;
s32           fd, error;

  file.errors = FileRequestUnkownError;
  file.handle = FILE_HANDLE_INVALID;
  file.size   = 0;
  file.exists = false;
  file.valid_actions = action;
  fd = -1;

  open_flags = fileActionToFlag(action);
  //Note(ern): if O_CREAT is not specified, then permissions is ignored
  mode_t permissions = 0666;

  if(0 && handle) {
    //TODO(ern): Debug why this is not working, openat returns 'File Not found' in most of the cases.
    fd = openat(FH_TO_INT(handle->folder.handle), STR_DATA(filepath), open_flags, permissions);
  } else {
    if(handle) {
      StrBuf temp = strbuf_alloc(.capacity=4096);
      strbuf_concatf(&temp, "%s"STRFMT, handle->path, STR_PRINT_ARGS(filepath));
      filepath = strbuf_get_str(&temp);
      fd = open(STR_DATA(filepath), open_flags, permissions);
      strbuf_release(&temp);
    } else {
      fd = open(STR_DATA(filepath), open_flags, permissions);
    }
  }

  if(fd == -1) {
      if(errno == ENOENT) {
          file.errors = FileRequestNotFound; 
      } else if(errno == EPERM || errno == EACCES) {
          file.errors = FileRequestNoPermission;
      } else if(errno == EISDIR ) {
          file.errors = FileRequestIsADirectory; 
      } else {
          file.errors = FileRequestUnkownError;
      }
  } else {
      FileHandle h = INT_TO_FH(fd);
      error = fstat(fd, &st);
      file.handle  = h;
      file.size    = st.st_size;
      file.errors  = FileRequestSuccess;
      file.exists  = true;
  }
  return file;
}

SocketType
osOpenSocket(s32 domain, s32 type, s32 protocol)
{
s32        sock, err, flags, option_value;
SocketType ret;

  flags = type;
  #if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
    flags |= SOCK_NONBLOCK;
    flags |= SOCK_CLOEXEC;
  #endif

  sock = socket(domain, flags, protocol);
  if(sock == -1) 
    return INT_TO_ST(sock);

  ret = INT_TO_ST(sock);
  
  if(type & SOCK_NONBLOCK == false) {
      bool result = osSocketSetNonBlocking(ret);
      if(result)
        result = osSocketSetCloseOnExec(ret);
      if(result == false) {
          osCloseSocket(ret);
          return SOCKET_INVALID;
      }
  } 

  return ret;
}

bool osSetSocketOption(SocketType sock, s32 option, void *option_val, size_t option_val_size)
{
    return setsockopt(ST_TO_INT(sock), 
                      SOL_SOCKET, 
                      option, 
           option_val, option_val_size) == 0;
}

RequestedFile
osOpenFile(Str filepath, enum FileAction action)
{
    return osOpenFile_impl(0, filepath, action);
}

bool osCloseFile(RequestedFile file)
{
bool result = true;
  if(file.exists && FH_TO_INT(file.handle) != -1) {
    int ret = close(FH_TO_INT(file.handle));
    if(ret == -1) result = false;
  }
  return result;
}

bool osSocketSetNonBlocking(SocketType s)
{
int flags;
	flags = fcntl(ST_TO_INT(s), F_GETFL, 0);
	if(flags == -1) {
		return false;
	}
	if(fcntl(ST_TO_INT(s), F_SETFL, flags | O_NONBLOCK) == -1) {
		return false;
	}
	return true;
}

bool osSocketSetCloseOnExec(SocketType s)
{
int flags;
	flags = fcntl(ST_TO_INT(s), F_GETFD, 0);
	if(flags == -1) {
		return false;
	}
	if(fcntl(ST_TO_INT(s), F_SETFD, flags | O_CLOEXEC) == -1) {
		return false;
	}
	return true;
}

SocketType 
osSocketGetServerSocket(Str addr,Str port)
{
SocketType s, ret;
int        optval = 1, err;
struct     addrinfo  hints;
struct     addrinfo  *result, *rp;
struct     sockaddr_storage  peer_addr;
cacheline_t ip_string, port_string;

	memset(&hints, 0, sizeof(hints));	
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;

  str_to_cstr(ip_string.b, sizeof(ip_string), addr);
  str_to_cstr(port_string.b, sizeof(port_string), port);

	err = getaddrinfo(ip_string.b, port_string.b, &hints, &result);
	if(err != 0) {
		return SOCKET_INVALID;
	}

	for(rp = result; rp != NULL; rp = rp->ai_next) {
    SocketType s = osOpenSocket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(ST_TO_INT(s) == -1) continue;

		if(bind(ST_TO_INT(s), rp->ai_addr, rp->ai_addrlen) == 0) 
    {
			ret = s;
			break;
		}
		close(ST_TO_INT(s));
	}

	freeaddrinfo(result);
	if(rp == NULL &&  ST_TO_INT(ret) == -1) {
		  return SOCKET_INVALID;
	}

  if(osSetSocketOption(ret, SO_REUSEADDR, &optval, sizeof(optval)) == false) {
      return SOCKET_INVALID;
  }

	if(listen(ST_TO_INT(ret), EB_LISTEN_BACKLOG)) {
		osCloseSocket(ret);
		return SOCKET_INVALID;
	}

	return ret;
}

u32
osSendFile(SocketType client, FileHandle file, u32 file_size)
{
off_t send_file_offset;
u32 ret;

    if(FH_TO_INT(file) == -1 || file_size <= 0) return 0;

    send_file_offset = 0;
    do {
      ret = sendfile(ST_TO_INT(client), FH_TO_INT(file), &send_file_offset, file_size);
    } while(ret == -1 && (errno == EWOULDBLOCK));
    return ret;
}

// TODO(ern): For now only epoll is implemented...
bool 
osAddSocketToMultiplexer(MultiplexerHandle multiplexer,
                         SocketType new_socket, 
                         enum MultiplexerFlags flags)
{

struct epoll_event ev;
int real_flags = 0;

  real_flags |= (flags & FlagRead)        ? EPOLLIN  : 0;
  real_flags |= (flags & FlagWrite)       ? EPOLLOUT : 0;
  real_flags |= (flags & FlagEdgeTrigger) ? EPOLLET  : 0;

  ev.events = real_flags;
  ev.data.fd = ST_TO_INT(new_socket);
  int ret = epoll_ctl(ST_TO_INT(multiplexer.e.epoll_socket), EPOLL_CTL_ADD, ev.data.fd, &ev);
  if(ret == -1) 
      return false;
  return true;
}

bool 
osRemoveSocketFromMultiplexer(MultiplexerHandle multiplexer, 
                              SocketType old_socket)
{
  int ret = epoll_ctl(ST_TO_INT(multiplexer.e.epoll_socket), EPOLL_CTL_DEL, ST_TO_INT(old_socket), NULL);
  if(ret  == -1) 
     return false;
  return true;
}

SocketType
osSocketAcceptConnection(SocketType server_sock)
{
SocketType ret;
   do {
      #if defined(accept4)
          ret = INT_TO_ST(accept4(ST_TO_INT(server_sock), 
                                  NULL, NULL, 
                                  SOCK_NONBLOCK|SOCK_CLOEXEC));
      #else 
         ret = INT_TO_ST(accept(ST_TO_INT(server_sock),
                                 NULL, NULL));
      #endif
   } while(ST_TO_INT(ret) == -1 && errno == EINTR);

  #if !defined(accept4)
    bool result = osSocketSetNonBlocking(ret);
    if(result) 
        result = osSocketSetCloseOnExec(ret);
    if(result == false) {
        osCloseSocket(ret);
        return SOCKET_INVALID;
    }
  #endif
  return ret;
}

SocketType osCreateMultiplexerSocket()
{
SocketType    epoll_fd = SOCKET_INVALID;
struct        epoll_event ev;

	epoll_fd = INT_TO_ST(epoll_create1(EPOLL_CLOEXEC));
	return epoll_fd;
}

bool 
osSocketIsValid(SocketType s)
{
  return s.socket != -1;
}

bool osCloseSocket(SocketType s)
{
  if(osSocketIsValid(s)) 
    close(ST_TO_INT(s));
}

bool osDestroyMultiplexerSocket(SocketType s)
{
  return osCloseSocket(s);
}

s32 osMultiplexerPoll(MultiplexerHandle h, s32 timeout)
{
s32 ret;

      if(h.valid == false) 
        return -1;
      if(h.handle_type == Epoll) {
          EpollHandle e;
          e = h.e;
          ret = epoll_wait(ST_TO_INT(e.epoll_socket),
                           e.events, e.events_count, 
                           timeout); 
      } else {
          return -1;
      }
      return ret;
}

bool
osGetSocketConnectionInfo(SocketType sock, Str ip_buf, s32 *port)
{
struct sockaddr_storage client_addr;
struct sockaddr_in      ipv4;
struct sockaddr_in6     ipv6;
socklen_t  client_addr_sz = sizeof(client_addr);

    getsockname(ST_TO_DATA(sock), (struct sockaddr*)&client_addr, &client_addr_sz);
    if(client_addr.ss_family == AF_INET) {
      ipv4 = *(struct sockaddr_in*)&client_addr;
      inet_ntop(client_addr.ss_family, (void*)&ipv4.sin_addr, STR_DATA(ip_buf), STR_SIZE(ip_buf));
      if(port) *port = (s32) ntohs(ipv4.sin_port);
    } else {
      ipv6 = *(struct sockaddr_in6*)&client_addr;
      inet_ntop(client_addr.ss_family, (void*)&ipv6.sin6_addr, STR_DATA(ip_buf), STR_SIZE(ip_buf));
      if(port) *port = (s32) ntohs(ipv6.sin6_port);
    }

    return true;
}

MultiplexerHandle
osGetMultiplexerHandle(ArenaAllocator *a, 
                       s32 type, s32 event_count)
{
MultiplexerHandle handle = {0};
    if(!a) 
      return handle;
    switch(type) {
    case Epoll:
        EpollHandle e;
        handle.handle_type = Epoll;
        e.epoll_socket = osCreateMultiplexerSocket();
        e.events = (struct epoll_event *)push_arena_array(a, struct epoll_event, event_count);
        e.events_count = event_count;
        handle.e = e;
        handle.valid = true;
        
    break;
    default:
      log("Not implemented");
      exit(EXIT_FAILURE);
    }
    return handle;
}

s32 osFullReadFromFile(RequestedFile f, StrBuf *buffer)
{
  if((f.valid_actions & ReadAction) == 0) {
    f.errors = FileRequestNoReadPermission;
    return -1;
  }

  SocketType s = {.socket=FH_TO_INT(f.handle)};
  return osFullReadFromSocket(s, buffer);
}

s32 osFullReadFromSocket(SocketType s, StrBuf *buffer)
{
s32   result;
u32   offset;
char  read_buffer[4096];

  do {
    result = read(ST_TO_INT(s), read_buffer, sizeof(read_buffer));
    if(result == 0) break;
    if(result == -1) {
        if(errno != EWOULDBLOCK) 
          break;
        if(errno == EAGAIN) 
          break;
        else
          return errno; 
    }
    strbuf_concat_sized(buffer, read_buffer, result, 0);
  } while(1);
  return 0;
}

s32 osFullWriteToSocket(SocketType s, Str buffer)
{
s32 result; 
u32 offset;

  offset = 0;
  do {
    Str slice = str_substr(buffer, offset, STR_SIZE(buffer) - offset);
    result = write(ST_TO_INT(s), STR_DATA(slice), STR_SIZE(slice));
    if(result == 0) break;
    if(result == -1) {
        if(errno != EWOULDBLOCK) 
          break;
        if(errno == EAGAIN) 
          break;
        else
          return errno; 
    }
    offset += result;
  } while(offset < STR_SIZE(buffer));
  return 0;
}

s32 osFullWriteToFile(RequestedFile f, Str buffer)
{
s32 result;
u32 offset;

  if((f.valid_actions & WriteAction) == 0) {
    f.errors = FileRequestNoWritePermission;
    return -1;
  }

  offset = 0;
  do {
    Str slice = str_substr(buffer, offset, STR_SIZE(buffer) - offset);
    result = write(FH_TO_INT(f.handle), STR_DATA(slice), STR_SIZE(slice));
    if(result == 0) break;
    if(result == -1) {
      if(errno != EWOULDBLOCK)
        break;
      if(errno == EAGAIN)
        break;
      else 
        return errno;
    }
    offset += result;
  } while(offset < STR_SIZE(buffer));
  return 0;
}

bool osSleep(u32 time_sec, u32 time_nsec)
{
struct timespec tm = {.tv_sec = time_sec, .tv_nsec = time_nsec};
  return nanosleep(&tm, NULL) == 0;
}

bool
osAppToBackgroundProcess(FolderHandle *work_dir)
{
pid_t         pid;
RequestedFile file;

  pid = fork();
  if(pid < 0) 
    return false;
  // This is the parent, just exit.
  if(pid > 0)
    exit(EXIT_SUCCESS);
  if(setsid() < 0) 
    return false;
  
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  pid = fork();
  if(pid < 0) 
    return false;
  if(pid > 0)
    exit(EXIT_SUCCESS);

  // New file permissions.
  umask(0);

  // Change current work directory to root folder
  chdir("/");
  
  // Close all open files.
  for(int i = sysconf(_SC_OPEN_MAX); i >= 0; i--) {
    close(i);
  }
  return true;
}

bool
osOpenDirectory(FolderHandle *restrict handle, Str folder_path)
{
RequestedFile directory;

  if(handle == 0) 
    return false;

  directory = osOpenFile(folder_path, FolderAction);
  handle->folder = directory;  
  if(directory.errors != FileRequestSuccess) {
    return false;
  }
  str_to_cstr(handle->path, sizeof(handle->path), folder_path);
  return true;
}

bool
osCloseDirectory(FolderHandle *restrict handle)
{
  if(handle == 0)
    return false;
  return osCloseFile(handle->folder);
}

RequestedFile 
osOpenFileFromFolder(FolderHandle *handle, 
                     Str filename, 
                     enum FileAction action)
{
RequestedFile file;
  file = osOpenFile_impl(handle, filename, action);
  return file;
}

bool
osSystemErrorToStr(Str s, SystemError e)
{
  return strerror_r(e, STR_DATA(s), STR_SIZE(s)) != 0;
}

bool
osSetupWorkingDirectory(FolderHandle *work_dir)
{
bool   success = false;
pid_t  pid;

  if(work_dir) {
    //TODO(ern): Str module should have utilities for this.
    char pidstr[16];
    pid = getpid();

    snprintf(pidstr, sizeof(pidstr), "%d", pid);
    Str pid_to_write = cstr(pidstr);

    RequestedFile pid_file;
    pid_file = osOpenFileFromFolder(work_dir, cstr_lit(PROG_NAME".pid"),
        WriteAction|CreateAction|TrucateAction); 
    osFullWriteToFile(pid_file, pid_to_write);
    success = osCloseFile(pid_file);
  }
  return success;
}
