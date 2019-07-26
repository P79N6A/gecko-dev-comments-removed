

















#ifndef __CMUTEX_H__
#define __CMUTEX_H__

typedef struct UMutex UMutex;







U_INTERNAL void U_EXPORT2 umtx_lock(UMutex* mutex); 





U_INTERNAL void U_EXPORT2 umtx_unlock (UMutex* mutex);

#endif

