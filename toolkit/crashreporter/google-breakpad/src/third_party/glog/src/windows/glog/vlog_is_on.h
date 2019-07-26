































































#ifndef BASE_VLOG_IS_ON_H_
#define BASE_VLOG_IS_ON_H_

#include "glog/log_severity.h"


#ifndef GOOGLE_GLOG_DLL_DECL
# if defined(_WIN32) && !defined(__CYGWIN__)
#   define GOOGLE_GLOG_DLL_DECL  __declspec(dllimport)
# else
#   define GOOGLE_GLOG_DLL_DECL
# endif
#endif

#if defined(__GNUC__)






#define VLOG_IS_ON(verboselevel)                                \
  __extension__  \
  ({ static google::int32* vlocal__ = &google::kLogSiteUninitialized;           \
     google::int32 verbose_level__ = (verboselevel);                    \
     (*vlocal__ >= verbose_level__) &&                          \
     ((vlocal__ != &google::kLogSiteUninitialized) ||                   \
      (google::InitVLOG3__(&vlocal__, &FLAGS_v,                         \
                   __FILE__, verbose_level__))); })
#else


#define VLOG_IS_ON(verboselevel) (FLAGS_v >= (verboselevel))
#endif









extern GOOGLE_GLOG_DLL_DECL int SetVLOGLevel(const char* module_pattern,
                                             int log_level);







extern google::int32 kLogSiteUninitialized;









extern GOOGLE_GLOG_DLL_DECL bool InitVLOG3__(
    google::int32** site_flag,
    google::int32* site_default,
    const char* fname,
    google::int32 verbose_level);

#endif  
