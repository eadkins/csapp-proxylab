#include <stdio.h>

#include "csapp.h"
#include "proxylib.h"
#include "cachelib.h"
#include "hashlib.h"

static const char *header_user_agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *header_accept = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *header_accept_encoding = "Accept-Encoding: gzip, deflate\r\n";
static const char *header_connection= "Connection: close\r\n";
static const char *header_proxy_connection= "Proxy-Connection: close\r\n";
static const char *header_empty_line = "\r\n";

void sigpipe_handler(int sig) {
  return;
}

int get_hash(REQUEST *request) {
  return crc32(request->request_content, strlen(request->request_content));
}

int parse_uri(char *uri, char *host, char *path, int *port) {
  char *hostbegin;
  char *hostend;
  char *pathbegin;
  int len;

  if (strncasecmp(uri, "http://", 7) != 0) {
    host[0] = '\0';
    return -1;
  }

  /* Extract the host name */
  hostbegin = uri + 7;
  hostend = strpbrk(hostbegin, " :/\r\n\0");
  len = hostend - hostbegin;
  strncpy(host, hostbegin, len);
  host[len] = '\0';

  /* Extract the port number */
  *port = DEFAULT_PORT; /* default */
  if (*hostend == ':')
    *port = atoi(hostend + 1);

  /* Extract the path */
  pathbegin = strchr(hostbegin, '/');
  if (pathbegin == NULL) {
    path[0] = '\0';
  } else {
    pathbegin++;
    strcpy(path, pathbegin);
  }

  return 0;
}

int parse_request(rio_t *client_rio, REQUEST *request) {
  int port;
  int header_contain_host;
  int request_content_pos;
  char buf[MAXLINE]; 
  char method[MAXLINE], uri[MAXLINE], host[MAXLINE], path[MAXLINE];
  char version[MAXLINE];
  ssize_t size;

  request_content_pos = 0;
  header_contain_host = 0;

  /* Read the request line from client */
  if (rio_readlineb(client_rio, buf, MAXLINE) < 0)
    return -1;  /* error */

  /* Parse the method, uri and version field */
  sscanf(buf, "%s %s %s", method, uri, version);
  
  /* Check whether it's GET request */
  if (strcasecmp(method, "GET")) {
    return -2;  /* error */
  }

  /* Parse URI from GET request */
  if (parse_uri(uri, host, path, &port) == -1) {
    return -3; /* error */
  }

  /* This is where the request will be forwarded */
  request->port = port;
  strcpy(request->host, host);

  /* Fill in method, uri and version fields of REQUEST */
  sprintf(request->request_content + request_content_pos, 
      "%s /%s %s\r\n", method, path, "HTTP/1.0");
  request_content_pos += strlen(request->request_content + request_content_pos);

  /* Read and process request header */
  while(1) {
    if ((size = rio_readlineb(client_rio, buf, MAXLINE)) < 0)
      return -3;  /* error */

    if (strcmp(buf, "\r\n") == 0)
      break;

    if (strncmp(buf, "User-Agent:", 11) &&
        strncmp(buf, "Accept:", 7) &&
        strncmp(buf, "Accept-Encoding:", 16) &&
        strncmp(buf, "Connection:", 11) &&
        strncmp(buf, "Proxy-Connection:", 17) ) {
     
      /* Test whether client's request header specifies Host: field */
      if ((strncmp(buf, "Host:", 5) == 0)) 
        header_contain_host = 1;
     
      sprintf(request->request_content + request_content_pos, 
          "%s", buf);
      request_content_pos += strlen(request->request_content + 
          request_content_pos);

    }
  }

  if (!header_contain_host) {
    sprintf(request->request_content+request_content_pos,
        "Host: %s\r\n", host);
    request_content_pos += strlen(request->request_content + 
        request_content_pos);
  }

  /* Add additional header */
  sprintf(request->request_content+request_content_pos, "%s%s%s%s%s%s", 
      header_user_agent, header_accept_encoding, header_accept,
      header_connection, header_proxy_connection, header_empty_line);
  request_content_pos += strlen(request->request_content + 
      request_content_pos);

  return 0;
}

int forward_request(int server_fd, REQUEST *request) {
  if (rio_writen(server_fd, request->request_content, 
        strlen(request->request_content)) < 0) 
    return -1;

  return 0;
}

int parse_response_header(rio_t *server_rio, RESPONSE *response) {
  char buf[MAXLINE]; 
  int response_header_pos;
  ssize_t size;

  /* Read the response header from server */
  response->content_len = 0;
  response_header_pos = 0;

  while(1) {
    if ((size = rio_readlineb(server_rio, buf, MAXLINE)) < 0)
      return -1;  /* error */

    //printf("Server returned: %s", buf);
    if (strcmp(buf, "\r\n") == 0)
      break;

    /* Test whether server's response header specifies content length */
    if ((strncmp(buf, "Content-Length:", 15) == 0)) {
      sscanf(buf, "Content-Length: %d", &response->content_len);
    }

    sprintf(response->response_header+response_header_pos, 
        "%s", buf);
    response_header_pos += strlen(response->response_header + 
        response_header_pos);
  }

  sprintf(response->response_header+response_header_pos, 
      "%s", header_empty_line);
  response_header_pos += strlen(response->response_header + 
      response_header_pos);

  return 0;
}

int read_response_object(rio_t *server_rio, RESPONSE *response) {
  ssize_t size;

  /* Read the object and store it in response's buf */
  size = rio_readnb(server_rio, response->content, response->content_len);
  if (size != response->content_len) 
    return -1;  /* error */

  return 0;
}

int forward_response_header(int client_fd, RESPONSE *response) {

  /* Forward the server's response header back to the client */
  if (rio_writen(client_fd, response->response_header,
        strlen(response->response_header)) < 0) 
    return -1;  /* error */

  return 0;
}


int test_cacheable(RESPONSE *response) {
  return ((response->content_len > 0) && (response->content_len < MAX_OBJECT_SIZE));
}

int forward_response_object(int client_fd, RESPONSE *response) {

  /* Forward the server's response object back to the client */
  if (rio_writen(client_fd, response->content, response->content_len) < 0) 
    return -1;  /* error */

  return 0;
}

int direct_forward_response_object(rio_t *server_rio, int client_fd) {
  char buf[MAXLINE];
  ssize_t size;

  while (1) {
     size = rio_readnb(server_rio, buf, MAXLINE);

     if (size < 0)
       return -1; /* error */
     else if (size == 0)
       break;

     if (rio_writen(client_fd, buf, size) < 0) 
       return -1; /* error */
  }

  return 0;
}


/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int client_fd, CACHE *cache) {
    int server_fd;
    rio_t client_rio, server_rio;

    REQUEST request;
    RESPONSE response;
    CACHE_OBJECT *cache_obj;
  
    rio_readinitb(&client_rio, client_fd);

    /* Read request line and headers */
    if (parse_request(&client_rio, &request) < 0) {
      clienterror(client_fd, "GET", "400", "Bad Request",
          "Error parsing the request");
      return;
    }

    dbg_printf("\n");
    dbg_printf("The following request will be sent to %s:%d :\n%s\n", 
        request.host, request.port, request.request_content);

    
    /* check whether this object is in cache */

    cache_obj = search_in_cache(cache, get_hash(&request));

    /* if in cache, directly send the objcet back to client */
    if (cache_obj != NULL) {
      dbg_printf("Cache hit with content_len = %d\n", cache_obj->content_len);

      response.content = cache_obj->content;
      response.content_len = cache_obj->content_len;
      strcpy(response.response_header, cache_obj->response_header);

      if (forward_response_header(client_fd, &response) < 0) {
        clienterror(client_fd, "GET", "502", "Bad Gateway",
            "Cannot forward response back to client");
        return;
      }

      if (forward_response_object(client_fd, &response) < 0) {
        clienterror(client_fd, "GET", "502", "Bad Gateway",
            "Cannot forward object back to client");
        return;
      }

      return;
    }

    /* Connect to server */
    if ((server_fd = open_clientfd(request.host, request.port)) < 0){
      clienterror(client_fd, "GET", "502", "Bad Gateway",
          "Cannot connect to the upstream server");
      return;
    }

    rio_readinitb(&server_rio, server_fd);

    /* Send request to server */
    if (forward_request(server_fd, &request) < 0) {
      clienterror(client_fd, "GET", "502", "Bad Gateway",
          "Cannot forward request to the upstream server");
      close(server_fd);
      return;
    }

    /* Parse response header from server */
    if (parse_response_header(&server_rio, &response) < 0) {
      clienterror(client_fd, "GET", "502", "Bad Gateway",
          "Cannot parse response from the upstream server");
      close(server_fd);
      return;
    }

    dbg_printf("The following response (content length = %d) will be sent back :\n%s\n",
        response.content_len, response.response_header);
    dbg_printf("The following response (content length = %d) will be sent back :\n",
        response.content_len);

    if (forward_response_header(client_fd, &response) < 0) {
      clienterror(client_fd, "GET", "502", "Bad Gateway",
          "Cannot forward response back to client");
      close(server_fd);
      return;
    }

    /* Check whether this object is cacheable */
    if (test_cacheable(&response)) {
      if ((response.content = malloc(response.content_len)) == NULL) {
        clienterror(client_fd, "GET", "502", "Bad Gateway",
            "Cannot allocate memory");
        close(server_fd);
        return;
      }

      if (read_response_object(&server_rio, &response) < 0) {
        clienterror(client_fd, "GET", "502", "Bad Gateway",
            "Cannot read object from the upstream server");
        close(server_fd);
        return;
      }

      if (forward_response_object(client_fd, &response) < 0) {
        clienterror(client_fd, "GET", "502", "Bad Gateway",
            "Cannot forward object back to client");
        close(server_fd);
        return;
      }

      add_to_cache(cache, get_hash(&request), response.response_header,
          response.content_len, response.content);

      free(response.content);

    } else {
      if (direct_forward_response_object(&server_rio, client_fd) < 0) {
        clienterror(client_fd, "GET", "502", "Bad Gateway",
            "Cannot directly forward object back to client");
        close(server_fd);
        return;
      }
    }

    close(server_fd);
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
}

