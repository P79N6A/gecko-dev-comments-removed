#ifndef stdbool_h
#define stdbool_h

#include <wtypes.h>





#ifndef __clang__
typedef BOOL _Bool;
#endif

#define bool _Bool
#define true 1
#define false 0

#define __bool_true_false_are_defined 1

#endif 
