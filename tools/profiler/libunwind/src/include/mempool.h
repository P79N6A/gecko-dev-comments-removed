
























#ifndef mempool_h
#define mempool_h






















#include <sys/types.h>

#include "libunwind_i.h"

#define sos_alloc(s)		UNWI_ARCH_OBJ(_sos_alloc)(s)
#define mempool_init(p,s,r)	UNWI_ARCH_OBJ(_mempool_init)(p,s,r)
#define mempool_alloc(p)	UNWI_ARCH_OBJ(_mempool_alloc)(p)
#define mempool_free(p,o)	UNWI_ARCH_OBJ(_mempool_free)(p,o)



struct mempool
  {
    pthread_mutex_t lock;
    size_t obj_size;		
    size_t chunk_size;		
    unsigned int reserve;	
    unsigned int num_free;	
    struct object
      {
	struct object *next;
      }
    *free_list;
  };




extern void *sos_alloc (size_t size);





extern void mempool_init (struct mempool *pool,
			  size_t obj_size, size_t reserve);
extern void *mempool_alloc (struct mempool *pool);
extern void mempool_free (struct mempool *pool, void *object);

#endif 
