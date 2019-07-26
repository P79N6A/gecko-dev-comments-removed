


#if A
# include "vm/Interpreter-inl.h"
# include "jsscriptinlines.h"
# include "js/Value.h"
# include "ds/LifoAlloc.h"
# include "jsapi.h"
# include <stdio.h>
# include "mozilla/HashFunctions.h"
#endif


#if B
# include "vm/Interpreter-inl.h"
# if C
#  include "js/Value.h"
#  if D
#   include "jsapi.h"
#  endif
#  include <stdio.h>
# endif
# include "mozilla/HashFunctions.h"
#endif

#include "jsobj.h"
#include "jsfun.h"      
#include "jsscript.h"
#include "jstypes.h"
