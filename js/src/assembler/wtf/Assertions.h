
























#ifndef assembler_wtf_Assertions_h
#define assembler_wtf_Assertions_h

#include "assembler/wtf/Platform.h"
#include "mozilla/Assertions.h"

#ifndef DEBUG
   



#  define ASSERT_DISABLED 1
#endif

#ifndef ASSERT
#define ASSERT(assertion) MOZ_ASSERT(assertion)
#endif
#define ASSERT_UNUSED(variable, assertion) do { \
    (void)variable; \
    ASSERT(assertion); \
} while (0)
#define ASSERT_NOT_REACHED() MOZ_ASSUME_UNREACHABLE()
#define CRASH() MOZ_CRASH()
#define COMPILE_ASSERT(exp, name) MOZ_STATIC_ASSERT(exp, #name)

#endif 
