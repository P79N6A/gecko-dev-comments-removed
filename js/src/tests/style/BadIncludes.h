
#if A
#include "tests/style/BadIncludes.h"    
#include "tests/style/BadIncludes2.h"   
#elif B
#include "BadIncludes2.h"               
#elif C
#include <tests/style/BadIncludes2.h>   
#elif D
#include "stdio.h"                      
#endif
