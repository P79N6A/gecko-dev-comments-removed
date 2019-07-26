
























































#include "config.h"

#undef GOOGLE_GLOG_DLL_DECL
#ifdef GOOGLE_GLOG_DLL_DECL_FOR_UNITTESTS
# define GOOGLE_GLOG_DLL_DECL  GOOGLE_GLOG_DLL_DECL_FOR_UNITTESTS
#else

# define GOOGLE_GLOG_DLL_DECL
#endif
