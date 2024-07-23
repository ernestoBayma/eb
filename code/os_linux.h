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
