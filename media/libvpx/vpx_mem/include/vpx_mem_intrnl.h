










#ifndef __VPX_MEM_INTRNL_H__
#define __VPX_MEM_INTRNL_H__
#include "vpx_ports/config.h"

#ifndef CONFIG_MEM_MANAGER
# if defined(VXWORKS)
#  define CONFIG_MEM_MANAGER  1 //include heap manager functionality,

# else
#  define CONFIG_MEM_MANAGER  0 //include heap manager functionality
# endif
#endif 

#ifndef CONFIG_MEM_TRACKER
# define CONFIG_MEM_TRACKER     1 //include xvpx_* calls in the lib
#endif

#ifndef CONFIG_MEM_CHECKS
# define CONFIG_MEM_CHECKS      0 //include some basic safety checks in

#endif

#ifndef USE_GLOBAL_FUNCTION_POINTERS
# define USE_GLOBAL_FUNCTION_POINTERS   0  //use function pointers instead of compiled functions.
#endif

#if CONFIG_MEM_TRACKER
# include "vpx_mem_tracker.h"
# if VPX_MEM_TRACKER_VERSION_CHIEF != 2 || VPX_MEM_TRACKER_VERSION_MAJOR != 5
#  error "vpx_mem requires memory tracker version 2.5 to track memory usage"
# endif
#endif

#define ADDRESS_STORAGE_SIZE      sizeof(size_t)

#ifndef DEFAULT_ALIGNMENT
# if defined(VXWORKS)
#  define DEFAULT_ALIGNMENT        32        //default addr alignment to use in


# else
#  define DEFAULT_ALIGNMENT        1
# endif
#endif

#if DEFAULT_ALIGNMENT < 1
# error "DEFAULT_ALIGNMENT must be >= 1!"
#endif

#if CONFIG_MEM_TRACKER
# define TRY_BOUNDS_CHECK         1         //when set to 1 pads each allocation,




#else
# define TRY_BOUNDS_CHECK         0
#endif 

#if TRY_BOUNDS_CHECK
# define TRY_BOUNDS_CHECK_ON_FREE 0          //checks mem integrity on every

# define BOUNDS_CHECK_VALUE       0xdeadbeef //value stored before/after ea.

# define BOUNDS_CHECK_PAD_SIZE    32         //size of the padding before and



#else
# define BOUNDS_CHECK_VALUE       0
# define BOUNDS_CHECK_PAD_SIZE    0
#endif 

#ifndef REMOVE_PRINTFS
# define REMOVE_PRINTFS 0
#endif


#if REMOVE_PRINTFS
# define _P(x)
#else
# define _P(x) x
#endif


#define align_addr(addr,align) (void*)(((size_t)(addr) + ((align) - 1)) & (size_t)-(align))

#endif 
