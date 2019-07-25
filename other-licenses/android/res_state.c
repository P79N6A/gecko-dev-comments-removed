















































#define ANDROID_CHANGES 1
#define MOZILLA_NECKO_EXCLUDE_CODE 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "arpa_nameser.h"
#include <netdb.h>
#include "resolv_private.h"
#include "resolv_cache.h"
#include <pthread.h>
#include <stdlib.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

static pthread_key_t   _res_key;
static pthread_once_t  _res_once;

typedef struct {
    int                    _h_errno;
    struct __res_state     _nres[1];
    unsigned               _serial;
    struct prop_info*      _pi;
    struct res_static      _rstatic[1];
} _res_thread;

static _res_thread*
_res_thread_alloc(void)
{
    _res_thread*  rt = malloc(sizeof(*rt));

    if (rt) {
        rt->_h_errno = 0;
        
        rt->_serial = 0;
        rt->_pi = (struct prop_info*) __system_property_find("net.change");
        if (rt->_pi) {
            rt->_serial = rt->_pi->serial;
        }
        if ( res_ninit( rt->_nres ) < 0 ) {
            free(rt);
            rt = NULL;
        } else {
            memset(rt->_rstatic, 0, sizeof rt->_rstatic);
        }
    }
    return rt;
}

static void
_res_static_done( res_static  rs )
{
    



    if (rs->hostf) {  
        fclose(rs->hostf);
        rs->hostf = NULL;
    }
    free(rs->servent.s_aliases);
}

static void
_res_thread_free( void*  _rt )
{
    _res_thread*  rt = _rt;

    _res_static_done(rt->_rstatic);
    res_ndestroy(rt->_nres);
    free(rt);
}

static void
_res_init_key( void )
{
    pthread_key_create( &_res_key, _res_thread_free );
}

static _res_thread*
_res_thread_get(void)
{
    _res_thread*  rt;
    pthread_once( &_res_once, _res_init_key );
    rt = pthread_getspecific( _res_key );
    if (rt == NULL) {
        if ((rt = _res_thread_alloc()) == NULL) {
            return NULL;
        }
        rt->_h_errno = 0;
        rt->_serial = 0;
        pthread_setspecific( _res_key, rt );
    }
    
    if (rt->_pi == NULL) {
        rt->_pi = (struct prop_info*) __system_property_find("net.change");
    }
    if (rt->_pi == NULL || rt->_serial == rt->_pi->serial) {
        return rt;
    }
    rt->_serial = rt->_pi->serial;
    
    if ( res_ninit( rt->_nres ) < 0 ) {
        free(rt);
        rt = NULL;
        pthread_setspecific( _res_key, rt );
    }
#ifdef USE_RESOLV_CACHE
    _resolv_cache_reset(rt->_serial);
#endif
    return rt;
}

struct __res_state _nres;

#if 0
struct resolv_cache*
__get_res_cache(void)
{
    _res_thread*  rt = _res_thread_get();

    if (!rt)
        return NULL;

    if (!rt->_cache) {
        rt->_cache = _resolv_cache_create();
    }
    return rt->_cache;
}
#endif

int*
__get_h_errno(void)
{
    _res_thread*  rt    = _res_thread_get();
    static int    panic = NETDB_INTERNAL;

    return rt ? &rt->_h_errno : &panic;
}

res_state
__res_get_state(void)
{
    _res_thread*  rt = _res_thread_get();

    return rt ? rt->_nres : NULL;
}

void
__res_put_state(res_state res)
{
    
    res=res;
}

res_static
__res_get_static(void)
{
    _res_thread*  rt = _res_thread_get();

    return rt ? rt->_rstatic : NULL;
}
