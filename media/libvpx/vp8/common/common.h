










#ifndef common_h
#define common_h 1

#include <assert.h>



#include "vpx_mem/vpx_mem.h"

#include "common_types.h"



#define vp8_copy( Dest, Src) { \
        assert( sizeof( Dest) == sizeof( Src)); \
        vpx_memcpy( Dest, Src, sizeof( Src)); \
    }



#define vp8_copy_array( Dest, Src, N) { \
        assert( sizeof( *Dest) == sizeof( *Src)); \
        vpx_memcpy( Dest, Src, N * sizeof( *Src)); \
    }

#define vp8_zero( Dest)  vpx_memset( &Dest, 0, sizeof( Dest));

#define vp8_zero_array( Dest, N)  vpx_memset( Dest, 0, N * sizeof( *Dest));


#endif  
