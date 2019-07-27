






#ifndef mozilla_mozalloc_oom_h
#define mozilla_mozalloc_oom_h

#include "mozalloc.h"





MFBT_API void mozalloc_handle_oom(size_t requestedSize);






typedef void (*mozalloc_oom_abort_handler)(size_t size);
MFBT_API void mozalloc_set_oom_abort_handler(mozalloc_oom_abort_handler handler);





#endif 
