














































#ifndef BASE_COMMANDLINEFLAGS_H__
#define BASE_COMMANDLINEFLAGS_H__

#include "config.h"
#include <string>
#include <string.h>               
#include <stdlib.h>               

#ifdef HAVE_LIB_GFLAGS

#include <gflags/gflags.h>

#else

#include "glog/logging.h"

#define DECLARE_VARIABLE(type, name, tn)                                      \
  namespace FLAG__namespace_do_not_use_directly_use_DECLARE_##tn##_instead {  \
  extern GOOGLE_GLOG_DLL_DECL type FLAGS_##name;                              \
  }                                                                           \
  using FLAG__namespace_do_not_use_directly_use_DECLARE_##tn##_instead::FLAGS_##name
#define DEFINE_VARIABLE(type, name, value, meaning, tn) \
  namespace FLAG__namespace_do_not_use_directly_use_DECLARE_##tn##_instead {  \
  GOOGLE_GLOG_DLL_DECL type FLAGS_##name(value);                              \
  char FLAGS_no##name;                                                        \
  }                                                                           \
  using FLAG__namespace_do_not_use_directly_use_DECLARE_##tn##_instead::FLAGS_##name


#define DECLARE_bool(name) \
  DECLARE_VARIABLE(bool, name, bool)
#define DEFINE_bool(name, value, meaning) \
  DEFINE_VARIABLE(bool, name, value, meaning, bool)


#define DECLARE_int32(name) \
  DECLARE_VARIABLE(GOOGLE_NAMESPACE::int32, name, int32)
#define DEFINE_int32(name, value, meaning) \
  DEFINE_VARIABLE(GOOGLE_NAMESPACE::int32, name, value, meaning, int32)



#define DECLARE_string(name)                                          \
  namespace FLAG__namespace_do_not_use_directly_use_DECLARE_string_instead {  \
  extern GOOGLE_GLOG_DLL_DECL std::string FLAGS_##name;                       \
  }                                                                           \
  using FLAG__namespace_do_not_use_directly_use_DECLARE_string_instead::FLAGS_##name
#define DEFINE_string(name, value, meaning)                                   \
  namespace FLAG__namespace_do_not_use_directly_use_DECLARE_string_instead {  \
  GOOGLE_GLOG_DLL_DECL std::string FLAGS_##name(value);                       \
  char FLAGS_no##name;                                                        \
  }                                                                           \
  using FLAG__namespace_do_not_use_directly_use_DECLARE_string_instead::FLAGS_##name

#endif  








#define GLOG_DEFINE_bool(name, value, meaning) \
  DEFINE_bool(name, EnvToBool("GLOG_" #name, value), meaning)

#define GLOG_DEFINE_int32(name, value, meaning) \
  DEFINE_int32(name, EnvToInt("GLOG_" #name, value), meaning)

#define GLOG_DEFINE_string(name, value, meaning) \
  DEFINE_string(name, EnvToString("GLOG_" #name, value), meaning)




#define EnvToString(envname, dflt)   \
  (!getenv(envname) ? (dflt) : getenv(envname))

#define EnvToBool(envname, dflt)   \
  (!getenv(envname) ? (dflt) : memchr("tTyY1\0", getenv(envname)[0], 6) != NULL)

#define EnvToInt(envname, dflt)  \
  (!getenv(envname) ? (dflt) : strtol(getenv(envname), NULL, 10))

#endif  
