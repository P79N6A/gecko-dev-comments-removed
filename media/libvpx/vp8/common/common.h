










#ifndef VP8_COMMON_COMMON_H_
#define VP8_COMMON_COMMON_H_

#include <assert.h>



#include "vpx_mem/vpx_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))



#define vp8_copy( Dest, Src) { \
        assert( sizeof( Dest) == sizeof( Src)); \
        memcpy( Dest, Src, sizeof( Src)); \
    }



#define vp8_copy_array( Dest, Src, N) { \
        assert( sizeof( *Dest) == sizeof( *Src)); \
        memcpy( Dest, Src, N * sizeof( *Src)); \
    }

#define vp8_zero( Dest)  memset( &Dest, 0, sizeof( Dest));

#define vp8_zero_array( Dest, N)  memset( Dest, 0, N * sizeof( *Dest));


#ifdef __cplusplus
}  
#endif

#endif
