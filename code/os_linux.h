#pragma once

#include <sys/epoll.h>
#include <sys/socket.h>
#include "core.h"

/* Declarations */
struct FileHandle {
  int fd;
};
struct SocketType {
  int socket;
};

#define FILE_HANDLE_INVALID (FileHandle){.fd=-1}
#define SOCKET_INVALID (SocketType){.socket=-1}

#define IS_ST_VALID(s) (bool)((s).socket != -1)
#define IS_FH_VALID(s) (bool)((s).fd != -1)
