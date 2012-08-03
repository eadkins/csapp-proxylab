#ifndef __CACHELIB_H__
#define __CACHELIB_H__


/* Macros and constants */

typedef struct cache_object{
  int hash;
  int content_len;
  time_t time_stamp;
  char *content;
  struct cache_object *prev_obj;
  struct cache_object *next_obj;
  char response_header[MAXLINE];
} CACHE_OBJECT;

typedef struct cache{
  int max_size;
  int size_used;
  CACHE_OBJECT *head;
} CACHE;

/* help routines */
CACHE_OBJECT *alloc_cache_obj(int content_len);
void free_cache_obj(CACHE_OBJECT *cache_obj);

/* interface for manipulating cache  */
/* return NULL if false, else, return ptr to the cache */
CACHE *build_cache(int max_size);
void clean_cache(CACHE *cache);
void free_cache(CACHE *cache);
void delete_LRU_cache_obj (CACHE *cache);

CACHE_OBJECT *add_to_cache(CACHE *cache, int hash, char *response_header, int size, char *buf);
CACHE_OBJECT *search_in_cache(CACHE *cache, int hash);


#endif /* __CACHELIB_H__ */
