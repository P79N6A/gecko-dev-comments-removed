





































#ifndef _RESOLV_CACHE_H_
#define _RESOLV_CACHE_H_

struct resolv_cache;  



extern struct resolv_cache*  __get_res_cache(void);



extern void   _resolv_cache_reset( unsigned  generation );

typedef enum {
    RESOLV_CACHE_UNSUPPORTED,  
                               
    RESOLV_CACHE_NOTFOUND,     
    RESOLV_CACHE_FOUND         
} ResolvCacheStatus;

extern ResolvCacheStatus
_resolv_cache_lookup( struct resolv_cache*  cache,
                      const void*           query,
                      int                   querylen,
                      void*                 answer,
                      int                   answersize,
                      int                  *answerlen );




extern void
_resolv_cache_add( struct resolv_cache*  cache,
                   const void*           query,
                   int                   querylen,
                   const void*           answer,
                   int                   answerlen );

#endif 
