
























#ifndef WTF_Assertions_h
#define WTF_Assertions_h

#include "Platform.h"
#include "mozilla/Assertions.h"

#ifndef DEBUG
   



#  define ASSERT_DISABLED 1
#endif

#define ASSERT(assertion) MOZ_ASSERT(assertion)
#define ASSERT_UNUSED(variable, assertion) do { \
    (void)variable; \
    ASSERT(assertion); \
} while (0)
#define ASSERT_NOT_REACHED() MOZ_NOT_REACHED("")
#define CRASH() MOZ_CRASH()
#define COMPILE_ASSERT(exp, name) MOZ_STATIC_ASSERT(exp, #name)

#endif  

