#include <stdio.h>

#include "csapp.h"
#include "cachelib.h"

/* internal helper routines */


CACHE_OBJECT *alloc_cache_obj(int content_len) {
  CACHE_OBJECT *cache_obj;

  cache_obj = malloc(sizeof(CACHE_OBJECT));
  if (cache_obj == NULL)
    return NULL;  /* error */

  cache_obj->content = malloc(content_len);
  if (cache_obj->content == NULL) {
    free(cache_obj);
    return NULL;  /* error */
  }

  return cache_obj;
}

void free_cache_obj(CACHE_OBJECT *cache_obj) {
  if (cache_obj == NULL)
    return;

  free(cache_obj->content);
  free(cache_obj);
}


/* external interfaces */
CACHE *build_cache(int max_size) {
  CACHE *cache;
  
  cache = malloc(sizeof(CACHE));
  if (cache == NULL)
    return NULL;  /* error */

  cache->max_size = max_size;
  cache->size_used = 0;
  cache->head = NULL;

  return cache;
}

void clean_cache(CACHE *cache) {
  if (cache == NULL)
    return;

  CACHE_OBJECT *curr_obj;
  CACHE_OBJECT *next_obj;
  
  curr_obj = cache->head;
  while (curr_obj != NULL) {
    next_obj = curr_obj->next_obj;
    free_cache_obj(curr_obj);
    curr_obj = next_obj;
  }

  cache->size_used = 0;
  cache->head = NULL;
}

void free_cache(CACHE *cache) {
  clean_cache(cache);
  free(cache);
}

void delete_LRU_cache_obj (CACHE *cache) {
  CACHE_OBJECT *LRU_obj, *curr_obj;
  time_t LRU_time_stamp;

  LRU_obj = NULL;
  curr_obj = cache->head;
  LRU_time_stamp = time(NULL);
  while (curr_obj != NULL) {
    if (curr_obj->time_stamp <= LRU_time_stamp)
      LRU_obj = curr_obj;

    curr_obj = curr_obj->next_obj;
  }

  if (LRU_obj != NULL) {
    /* remove the LRU_obj from the linked list */
    LRU_obj->prev_obj->next_obj = LRU_obj->next_obj;
    LRU_obj->next_obj->prev_obj = LRU_obj->prev_obj;
    /* expand the usable space */
    cache->size_used -= LRU_obj->content_len;
    free_cache_obj(LRU_obj);
  }
}

/* external interface routines */
CACHE_OBJECT *add_to_cache(CACHE *cache, int hash, char *response_header, int size, char *buf){
  /* Test whether the space is enough for this object */
  CACHE_OBJECT *cache_obj;

  if (size > cache->max_size)
    return NULL;  /* error */

  while ((cache->size_used + size) > cache->max_size) {
    delete_LRU_cache_obj(cache);
  }

  cache_obj = alloc_cache_obj(size);
  if (cache_obj == NULL)
    return NULL;

  cache_obj->hash = hash;
  cache_obj->content_len = size;
  cache_obj->time_stamp = time(NULL);
  strcpy(cache_obj->response_header, response_header);
  memcpy(cache_obj->content, buf, size);

  /* insert the new cache object into the list's head */
  cache_obj->prev_obj = NULL;
  cache_obj->next_obj = cache->head;
  if (cache->head != NULL)
    cache->head->prev_obj = cache_obj;
  cache->head = cache_obj;

  return cache_obj;
}

CACHE_OBJECT *search_in_cache(CACHE *cache, int hash) {
  CACHE_OBJECT *curr_obj;

  curr_obj = cache->head;
  while (curr_obj != NULL) {
    if (curr_obj->hash == hash)
      break;
    curr_obj = curr_obj->next_obj;
  }


  if (curr_obj != NULL) {
    /* update the timestamp */
    curr_obj->time_stamp = time(NULL);
  }

  return curr_obj;
}
