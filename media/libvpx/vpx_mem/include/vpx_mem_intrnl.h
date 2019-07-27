










#ifndef VPX_MEM_INCLUDE_VPX_MEM_INTRNL_H_
#define VPX_MEM_INCLUDE_VPX_MEM_INTRNL_H_
#include "./vpx_config.h"

#define ADDRESS_STORAGE_SIZE      sizeof(size_t)

#ifndef DEFAULT_ALIGNMENT
# if defined(VXWORKS)
#  define DEFAULT_ALIGNMENT        32        /*default addr alignment to use in
calls to vpx_* functions other
than vpx_memalign*/
# else
#  define DEFAULT_ALIGNMENT        (2 * sizeof(void*))  /* NOLINT */
# endif
#endif


#define align_addr(addr,align) (void*)(((size_t)(addr) + ((align) - 1)) & (size_t)-(align))

#endif  
