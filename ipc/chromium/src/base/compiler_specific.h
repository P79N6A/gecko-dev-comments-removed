



#ifndef BASE_COMPILER_SPECIFIC_H_
#define BASE_COMPILER_SPECIFIC_H_

#include "build/build_config.h"

#if defined(COMPILER_MSVC)














#define MSVC_SUPPRESS_WARNING(n) __pragma(warning(suppress:n))



#define MSVC_PUSH_DISABLE_WARNING(n) __pragma(warning(push)) \
                                     __pragma(warning(disable:n))




#define MSVC_PUSH_WARNING_LEVEL(n) __pragma(warning(push, n))


#define MSVC_POP_WARNING() __pragma(warning(pop))

#define MSVC_DISABLE_OPTIMIZE() __pragma(optimize("", off))
#define MSVC_ENABLE_OPTIMIZE() __pragma(optimize("", on))










#define ALLOW_THIS_IN_INITIALIZER_LIST(code) MSVC_PUSH_DISABLE_WARNING(4355) \
                                             code \
                                             MSVC_POP_WARNING()

#else  

#define MSVC_SUPPRESS_WARNING(n)
#define MSVC_PUSH_DISABLE_WARNING(n)
#define MSVC_PUSH_WARNING_LEVEL(n)
#define MSVC_POP_WARNING()
#define MSVC_DISABLE_OPTIMIZE()
#define MSVC_ENABLE_OPTIMIZE()
#define ALLOW_THIS_IN_INITIALIZER_LIST(code) code

#endif  


#if defined(COMPILER_GCC)
#define ALLOW_UNUSED __attribute__((unused))
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else  
#define ALLOW_UNUSED
#define WARN_UNUSED_RESULT
#endif

#endif  
