#if !defined(EB_HTTP_HEADER)
#define EB_HTTP_HEADER

#include "core.h"
#include "util.h"
#include "str.h"

global_var Str NOT_FOUND_HTML = cstr_lit( 
"<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
    "    <title>404 Not Found</title>\n"
    "    <style>\n"
    "        body {\n"
    "            margin: 0;\n"
    "            padding: 0;\n"
    "            display: flex;\n"
    "            justify-content: center;\n"
    "            align-items: center;\n"
    "            height: 100vh;\n"
    "            background-color: #f3f3f3;\n"
    "            font-family: 'Arial', sans-serif;\n"
    "        }\n"
    "        .container {\n"
    "            text-align: center;\n"
    "        }\n"
    "        h1 {\n"
    "            font-size: 5em;\n"
    "            margin: 0;\n"
    "            color: #ff6f61;\n"
    "        }\n"
    "        p {\n"
    "            font-size: 1.5em;\n"
    "            color: #333;\n"
    "        }\n"
    "        a {\n"
    "            display: inline-block;\n"
    "            margin-top: 20px;\n"
    "            padding: 10px 20px;\n"
    "            font-size: 1em;\n"
    "            color: #fff;\n"
    "            background-color: #ff6f61;\n"
    "            text-decoration: none;\n"
    "            border-radius: 5px;\n"
    "            transition: background-color 0.3s;\n"
    "        }\n"
    "        a:hover {\n"
    "            background-color: #ff3f31;\n"
    "        }\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <div class=\"container\">\n"
    "        <h1>404</h1>\n"
    "        <p>Oops! The page you're looking for doesn't exist.</p>\n"
    "        <a href=\"/\">Go Home</a>\n"
    "    </div>\n"
    "</body>\n"
    "</html>\n"
);

#define HTTP_1_1_HEADER(status) "HTTP/1.1 " status"\r\n"

#define STATUS_OK "200 OK"
#define STATUS_NOT_FOUND "404 Not Found"
#define STATUS_INTERNAL_ERROR "500 Internal Error"
#define STATUS_NOT_IMPLEMENTED "501 Not Implemented"

#define HEADER_LINE_END "\r\n"
#define HEADER_CONTENT_TYPE "Content-Type: %s" HEADER_LINE_END
#define HEADER_CONTENT_LEN  "Content-Length: %u" HEADER_LINE_END

#define STATUS_OK_INTEGER 200
#define STATUS_NOT_FOUND_INTEGER 404
#define STATUS_INTERNAL_ERROR_INTEGER 500

enum {
  METHOD_GET,
  METHOD_COUNT
};

typedef struct ValidFiletype {
  KeyValue http_type;
  int      http_type_len;
} ValidFiletype;

global_var ValidFiletype FILE_TYPES[] = 
{
  {(KeyValue){"*", "application/octet-stream"}, strlen("application/octet-stream")},
  {(KeyValue){"html", "text/html"}, strlen("text/html")},
  {(KeyValue){"css", "text/css"}, strlen("text/css")},
  {(KeyValue){"js", "text/javascript"}, strlen("text/javascript")},
  {0}
};
global_var char *METHODS_IMPLEMENTED[] = 
{
    "GET",
    NULL
};

#endif
