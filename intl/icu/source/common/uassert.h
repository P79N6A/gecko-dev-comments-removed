


















#ifndef U_ASSERT_H
#define U_ASSERT_H

#include "unicode/utypes.h"
#if U_DEBUG
#   include <assert.h>
#   define U_ASSERT(exp) assert(exp)
#else
#   define U_ASSERT(exp)
#endif
#endif


