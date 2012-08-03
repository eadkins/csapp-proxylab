#ifndef __PROXYLIB_H__
#define __PROXYLIB_H__

#include "cachelib.h"

/* Macros and constants */

/* If you want debugging output, use the following macro.  When you hand
 *  * in, remove the #define DEBUG line. */
//#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

#ifdef DEBUG
#define PRINT_LINE_NUM do {printf("%d\n", __LINE__);} while(0)
#else
#define PRINT_LINE_NUM
#endif

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define DEFAULT_PORT 80

typedef struct request{
  int port;
  char host[MAXLINE];
  char request_content[MAXLINE]; 
} REQUEST;

typedef struct response{
  int content_len;
  char response_header[MAXLINE]; 
  char *content;
} RESPONSE;

/* signal handlers */
void sigpipe_handler(int sig);

/* help routines */

int get_hash(REQUEST *request);

int parse_uri(char *uri, char *host, char *path, int *port);
int parse_request(rio_t *client_rio, REQUEST *request);
int forward_request(int server_fd, REQUEST *request);
int parse_response_header(rio_t *server_rio, RESPONSE *response);
int read_response_object(rio_t *server_rio, RESPONSE *response);
int forward_response_header(int client_fd, RESPONSE *response);
int test_cacheable(RESPONSE *response);
int forward_response_object(int client_fd, RESPONSE *response);
int direct_forward_response_object(rio_t *server_rio, int client_fd);


void doit(int fd, CACHE *cache);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);



#endif /* __PROXYLIB_H__ */
