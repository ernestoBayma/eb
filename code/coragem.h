#pragma once

#include "core.h"
#include "allocator.h"
#include "str_list.h"
#include "str_own.h"
#include "str.h"
#include "str_buf.h"
#include "http.h"
#include "event.h"
#include "logger.h"

typedef struct ServerConf ServerConf;
struct ServerConf 
{
    ArenaAllocator    *arena[2];
    Str               server_addr;
    Str               port;
    FolderHandle      root_folder;
    FolderHandle      work_dir; 
    Logger            info_log;
    Logger            error_log;
    Logger            request_log;
    u32               throttle;
    EventHandle       server;
};

