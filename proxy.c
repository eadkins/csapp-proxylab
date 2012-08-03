#include <stdio.h>

#include "csapp.h"
#include "proxylib.h"

int main(int argc, char **argv) {
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  Signal(SIGPIPE,  sigpipe_handler);   

  port = atoi(argv[1]);

  listenfd = Open_listenfd(port);

  if (listenfd < 0)
    return -1;

  CACHE *cache = build_cache(MAX_CACHE_SIZE);
  if (cache == NULL) {
    printf("Error: Cannot build cache\n");
    return -1;
  }

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, (unsigned int *)&clientlen);
    doit(connfd, cache);
    Close(connfd);

  }

  free_cache(cache);

  return 0;
}
