#include "jstypes.h"

void GreenFunc();

typedef void (JS_REQUIRES_STACK *RedFuncPtr)();

RedFuncPtr Test()
{
  
  RedFuncPtr p = GreenFunc;
  return p;
}
