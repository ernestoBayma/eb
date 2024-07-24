/*
 *
 *  Copyright (C) 2024 Ernesto Bayma
 * - Need to fix:
 *  Scrach Allocation together with main allocation.
 *  Chunked transfer.
 *  Implement other HTTP methods.
 *  HTTPS.
 *
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#include "coragem.h"

#define EB_STR_IMPLEMENTATION
#include "str.h"

#define EB_STR_BUF_IMPLEMENTATION
#include "str_buf.h"

#define EB_LOGGER_IMPLEMENTATION
#include "logger.h"

#define EB_STR_LIST_IMPLEMENTATION
#include "str_list.h"

#define EB_STR_OWN_IMPLEMENTATION
#include "str_own.h"

global_var ServerConf global_server;

global_var KeyValue usageOptions[] = {
    {"--help", "Display this help message"},
    {"--config", "Configuration file of the server"},
    {"--daemon", "Makes server run as background process"},
    {0,0}
};


priv_func ArenaAllocator *serverGetMainAllocator(ServerConf *server)
{
  return server->arena[0];
}

priv_func ArenaAllocator *serverGetBackupAllocator(ServerConf *server)
{
  return server->arena[1];
}

priv_func void serverSetMainAllocator(ServerConf *server, ArenaAllocator *arena)
{
    server->arena[0] = arena;
}

priv_func void serverSetBackupAllocator(ServerConf *server, ArenaAllocator *arena)
{
    server->arena[1] = arena;
}

priv_func void read_config_file(ServerConf *server, ArenaAllocator *arena, Str filepath);
priv_func StrList str_list_build_from_lines(Str input_lines, ArenaAllocator *arena);

priv_func RequestedFile 
openRequestedFile(Str filepath, FolderHandle *local_folder) 
{
RequestedFile     file;

  file = osOpenFileFromFolder(local_folder, filepath, ReadAction);
	return file;
}

priv_func Logger
openLog(Str filepath, Str log_name)
{
RequestedFile log = {0};
Logger        l = {0};
char          fullpath[EB_MAX_PATH_SIZE] = {0};
  
  if(STR_SIZE(filepath) > 0) {
    if(STR_SIZE(log_name) > 0) {
      snprintf(fullpath, sizeof(fullpath), STRFMT"/"STRFMT, 
           STR_PRINT_ARGS(filepath), STR_PRINT_ARGS(log_name));
    } else {
      str_to_cstr(fullpath, sizeof(fullpath), filepath);
    }
    log = osOpenFile(cstr(fullpath), WriteAction|CreateAction|AppendAction);
    l   = INIT_LOGGER(log); 
  }
  return l;
}

StrBuf readFile(ArenaAllocator *arena, RequestedFile file)
{
StrBuf buf;

    buf = strbuf_alloc(.shared_arena=arena);
    osFullReadFromFile(file, &buf);
    return buf;
}

bool sendToClient(StrBuf *buf, SocketType client)
{
Str str;
    str = strbuf_get_str(buf);
    if(!STR_PTR_VALID(str))
        return false;
    osFullWriteToSocket(client, str);
    return true;
}


priv_func RequestedFile closeRequestedFile(RequestedFile file)
{
  osCloseFile(file);
  file.handle = FILE_HANDLE_INVALID;
  file.exists = 0;
  file.size = 0;
  return file;
}

priv_func void send_get_header(SocketType client, Str response)
{
    osFullWriteToSocket(client, response);
}

priv_func void print_file_error(RequestedFile file)
{
cacheline_t e = {0};
Str s_err = cstr_static(e.b);
    humanRedableFileRequestError(s_err, file.errors);
    log("[DEBUG] File returned with status "STRFMT"\n", STR_PRINT_ARGS(s_err));
}

priv_func int handle_get_response(ServerConf *server, Str s, SocketType  client)
{
RequestedFile   file;
char            filetype[256] = {0};
int             i, ret;
enum            {GET=0,PATH,HTTP_VERSION,ELEMENTS_TOTAL};
Str             line[ELEMENTS_TOTAL];
char            bar[] = "/";
ArenaAllocator  *arena;
ScrachAllocator  scrach;

  arena  = serverGetBackupAllocator(server);
  scrach = scrachNew(arena);

  ret = STATUS_INTERNAL_ERROR_INTEGER;
  str_split_all(ELEMENTS_TOTAL, line, s, " ");

  if(STR_SIZE(line[PATH]) == 0 || str_is_match(line[PATH], "/"))
    line[PATH] = cstr_lit("index.html");
  
  Str file_name = str_trim_start(line[PATH], bar);

  DeferLoop((file = openRequestedFile(file_name, &server->root_folder)), (file = closeRequestedFile(file), scrachEnd(scrach))) {
    bool found = false;
    for(i = 0; !found && FILE_TYPES[i].http_type_len; i++) {
        if(str_contains(file_name, cstr(FILE_TYPES[i].http_type.key))) {
          memcpy(filetype, FILE_TYPES[i].http_type.value, FILE_TYPES[i].http_type_len);
          found = true;
        }
    }
    if(!found) {
      memcpy(filetype, FILE_TYPES[0].http_type.value, FILE_TYPES[0].http_type_len);
    }

    StrBuf response = strbuf_alloc(.shared_arena=scrach.arena,.capacity=4096);
    if(file.errors == FileRequestSuccess) {
      strbuf_concatf(&response, HTTP_1_1_HEADER(STATUS_OK) HEADER_CONTENT_TYPE HEADER_CONTENT_LEN HEADER_LINE_END HEADER_LINE_END, filetype, file.size);
      ret = STATUS_OK_INTEGER;
    } else if(file.errors == FileRequestNotFound) {
      strbuf_concat(&response, HTTP_1_1_HEADER(STATUS_NOT_FOUND) HEADER_LINE_END HEADER_LINE_END, 0);
      ret = STATUS_NOT_FOUND_INTEGER;
    } else {
      print_file_error(file);
      strbuf_concat(&response, HTTP_1_1_HEADER(STATUS_INTERNAL_ERROR) HEADER_LINE_END HEADER_LINE_END, 0);
    }

    Str header = strbuf_get_str(&response);
    send_get_header(client, header);

    switch(ret) {
    case STATUS_NOT_FOUND_INTEGER:
      osFullWriteToSocket(client, NOT_FOUND_HTML);
      break;
    default:
      if(file.exists) {
        StrBuf file_contents = readFile(scrach.arena, file);
        sendToClient(&file_contents, client);
      }
    break;
    }
  }
	return ret;
}

void
log_error(Event event, void *server_ctx)
{
ServerConf      *conf;
ScrachAllocator  temp;
char             error_buff[1024];
Str              str;

  conf = (ServerConf*)server_ctx;
  temp = scrachNew(serverGetMainAllocator(conf));
  str  = cstr_static(error_buff);

  osSystemErrorToStr(str, event.err);
  log_putsf(conf->info_log, temp.arena, Error, 
           "Client had a error: "STRFMT"\n", STR_PRINT_ARGS(str));
  ScrachEnd__debug(temp);
}

/* Note(ern):
 * Reads and Writes need to be handled diferently,
 * for Reading we need to have a static buffer of ?4096? and
 * read util we can fill the buffer, if would block then go
 * to next request, util that request can't fill the buffer anymore.
 * Writting depends of the type and size of the response, but it should be 
 * the same idea. If is a really big file we should send it chunked.
 */ 
enum CallbackResult
handle_client(Event event, void *server_ctx)
{
char            data_from_client[1024];
int             ret, method_cursor;
Str             data_str;
ServerConf      conf;
StrList         http_header;
ArenaAllocator *temp;
ScrachAllocator scrach;

  conf    = *((ServerConf*)server_ctx);
  scrach  = scrachNew(serverGetMainAllocator(&conf));
  temp    = scrach.arena;
  Str client_data = strbuf_get_str(&event.ev_buff);
  
  ret = -1;
  http_header = str_list_build_from_lines(client_data, temp);
  if(http_header.count > 0) {
    StrNode *first_line = http_header.first;
    if(!first_line) {
        log_puts(conf.info_log, Error, "Could not read http header\n");
        ret = CallbackCloseClient | CallbackError;
        MemoryZeroStruct(&http_header);
        ScrachEnd__debug(scrach);
        return ret;        
    }
    for(method_cursor = 0; METHODS_IMPLEMENTED[method_cursor]; method_cursor++) {
      Str method = str_find_first(first_line->value, METHODS_IMPLEMENTED[method_cursor]);
      if(STR_SIZE(method)) {
        if(method_cursor == METHOD_GET) {
          ret = handle_get_response(&conf, first_line->value, event.client);          
          break;
        }
      }
    }
    if(method_cursor == METHOD_COUNT) {
      log_puts(conf.info_log, Warning, cstr_lit("Unplemented HTTP 1.1 method\n"));
      log_putsf(conf.request_log, temp, Error, " IP: "STRFMT":%d => %d - '"STRFMT"'\n", STR_PRINT_ARGS(event.info.client_ip), event.info.port, ret, STR_PRINT_ARGS(first_line->value));
    } else {
      log_putsf(conf.request_log, temp, Info, " IP: "STRFMT":%d => %d - '"STRFMT"'\n", STR_PRINT_ARGS(event.info.client_ip), event.info.port, ret, STR_PRINT_ARGS(first_line->value));
    }
  } else {
      log_puts(conf.info_log, Error, cstr_lit("Data from client came empty..\n"));
      ret = CallbackSuccess | CallbackKeepClient;
  }
  
  if(ret == -1) 
    ret = CallbackCloseClient | CallbackSuccess;

  MemoryZeroStruct(&http_header);
  ScrachEnd__debug(scrach);

  return ret;
}

priv_func bool init_server(ServerConf *s, char **arg_values, int arg_count)
{
ArenaAllocator *temp;
StrList         args;
StrNode*        arg;
bool            valid_args, background_process, has_config;
Str             arg_message_error, config_file_path;
ScrachAllocator scrach;

  scrach              = scrachNew(serverGetBackupAllocator(s));
  temp                = scrach.arena;
  args                = str_list_create(temp, arg_values, arg_count);
  background_process  = false;
  has_config          = false;

  // ern: Command line arguments
  {
    arg_message_error = STR_INVALID;
    arg = str_list_contains(&args, cstr_lit("--help"));
    if(arg) {
      printUsage(PROG_NAME, VERSION, stdout, usageOptions); 
    }
    arg = str_list_contains(&args, cstr_lit("--config"));
    if(arg) {
        if(arg->next) {
          StrNode *config = arg->next;
          config_file_path = config->value;
          has_config = true;
        } else {
          arg_message_error = cstr_lit("Missing configuration file path");
        }
    }
    arg = str_list_contains(&args, cstr_lit("--daemon"));
    if(arg) {
      background_process = true;
    }
  }

  if(STR_PTR_VALID(arg_message_error)) {
      fprintf(stderr, "Error: "STRFMT"\n", STR_PRINT_ARGS(arg_message_error));
      return false;
  }

  if(background_process) {
      if(!osAppToBackgroundProcess()) {
        fprintf(stderr, "Error: Was not possible to become a background process. Leaving...");
        return false;
      } 
  }

  if(has_config) {
      read_config_file(s, temp, config_file_path);
  }

  osSetupWorkingDirectory(&s->work_dir);

  if(!STR_SIZE(s->port))
    s->port = cstr_lit(DEFAULT_PORT);

	if(!STR_SIZE(s->server_addr))
    s->server_addr = cstr_lit(DEFAULT_ADDR);

  log("Starting server at "STRFMT":"STRFMT"\n", STR_PRINT_ARGS(s->server_addr),STR_PRINT_ARGS(s->port));

  EventOptions opts = {
      .event_type       = EventTcpServer,
      .server_ip        = s->server_addr,
      .server_port      = s->port,
      .user_defined_ptr = s,
      .callback         = handle_client,
      .callback_error   = log_error,
      .multiplexer_type = Epoll,
      .sleep_nsec       = s->throttle,
      .event_count      = 10,
      .timeout          = 1000 
  };

  s->server = eventInitHandle(serverGetMainAllocator(s), opts);
  ScrachEnd__debug(scrach); 
  return true;
}

priv_func void main_loop(ServerConf *s)
{
  eventQueueRunLoop(s->server); 
}

priv_func void parse_config_file(ServerConf *server,
                       StrList *lines)
{
enum              {KEY=0, VALUE, TOTAL};
Str               k_v[TOTAL];
enum              {Error=(1<<0), Request=(1<<1), Info=(1<<2)};
enum              {ErPtr=0, RePtr, InPtr, PtrCnt};
enum              {RootFolder=(1<<0),WorkFolder=(1<<1)};
enum              {RPtr=0,WPtr,FPtrCnt};
s32               logs, folders;
Str              log_names[PtrCnt], folder_paths[FPtrCnt];

  logs = 0;
  folders = 0;
  STR_LIST_FOR_EACH(lines, curr) {
      if(str_starts_with(curr->value, cstr_lit("#"))) {
          continue;
      }
      if(str_split_all(TOTAL, k_v, curr->value, cstr_lit("="))) {
          Str key_trimmed   = str_trim(k_v[KEY], " ");
          Str value_trimmed = str_trim(k_v[VALUE], " ");
          
          if(str_is_match_no_case(key_trimmed, cstr_lit("server_port"))) {
              if(!STR_SIZE(server->port)) {
                server->port = str_substr(value_trimmed, 0, STR_SIZE(value_trimmed));
              }
          } else if(str_is_match_no_case(key_trimmed, cstr_lit("server_addr"))) {
              if(!STR_SIZE(server->server_addr)) {
                server->server_addr = str_substr(value_trimmed, 0, STR_SIZE(value_trimmed));
              }
          } else if(str_is_match_no_case(key_trimmed, cstr_lit("server_throttle"))) {
              if(!server->throttle)
                server->throttle = Seconds_Nano(CstrToInteger(u32, STR_DATA(value_trimmed)));
          } else if(str_is_match_no_case(key_trimmed, cstr_lit("server_root_folder"))) {
                  folders |= RootFolder;
                  folder_paths[RPtr] = str_substr(value_trimmed, 0, STR_SIZE(value_trimmed));
          } else if(str_is_match_no_case(key_trimmed, "server_error_log")) {
                logs |= Error;
                log_names[ErPtr] = str_substr(value_trimmed, 0, STR_SIZE(value_trimmed));
          } else if(str_is_match_no_case(key_trimmed, "server_request_log")) {
                logs |= Request;
                log_names[RePtr] = str_substr(value_trimmed, 0, STR_SIZE(value_trimmed));
          } else if(str_is_match_no_case(key_trimmed, "server_info_log")) {
                logs |= Info;
                log_names[InPtr] = str_substr(value_trimmed, 0, STR_SIZE(value_trimmed));
          } else if(str_is_match_no_case(key_trimmed, "server_working_directory")) {
                folders |= WorkFolder;
                folder_paths[WPtr] = str_substr(value_trimmed, 0, STR_SIZE(value_trimmed));
          }
      } 
  } 

  if(logs & Info)   
    server->info_log    = openLog(log_names[InPtr], STR_INVALID);
  if(logs & Error)
    server->error_log   = openLog(log_names[ErPtr], STR_INVALID);
  if(logs & Request)
    server->request_log = openLog(log_names[RePtr], STR_INVALID);

  if(folders & RootFolder)
      osOpenDirectory(&server->root_folder, folder_paths[RPtr]);
  else
      osOpenDirectory(&server->root_folder, cstr(DEFAULT_ROOT_FOLDER));
  if(folders & WorkFolder)
      osOpenDirectory(&server->work_dir, folder_paths[WPtr]);
}

priv_func StrList str_list_build_from_lines(Str input_lines, ArenaAllocator *arena)
{
Str     line;
StrList lines;
char    end_line[] = "\r\n";

  lines = str_list_init_empty(arena);
  do {
    line = str_split_line(&input_lines, end_line);
    StrOwn l = str_own_copy(arena, line); 
    str_list_add_from_str(arena, &lines, l.str); 
  } while(STR_PTR_VALID(line));
  return lines;
}

priv_func void read_config_file(ServerConf *server, ArenaAllocator *arena, Str filepath)
{
RequestedFile   file;
StrList         config_lines;
StrBuf           file_contents;
Str              file_str;
s32              result;

  file = osOpenFile(filepath, ReadAction);
  if(file.errors != FileRequestSuccess) {
    goto close_file;
  }
  if(file.size <= 0) {
    goto close_file;
  }

  file_contents = strbuf_alloc(.shared_arena=arena,.capacity=file.size);
  result = osFullReadFromFile(file, &file_contents);
  if(result != 0) {
      log("Failed reading from file error from OS [%d]\n", result);
  } else {
      file_str = strbuf_get_str(&file_contents);
      config_lines = str_list_build_from_lines(file_str, arena);          
      parse_config_file(server, &config_lines);
  }

close_file:
  osCloseFile(file);
}

void app_entry_point(int arg_count, char *arg_values[])
{
StrList          arg_list;

  serverSetMainAllocator(&global_server, arenaNew(.allocator_name=cstr_lit("MainAllocator")));
  serverSetBackupAllocator(&global_server, arenaNew(.allocator_name=cstr_lit("BackupAllocator")));

  if(init_server(&global_server, arg_values, arg_count)) {
    main_loop(&global_server);
  }
}
