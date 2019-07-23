#include "nscore.h"

struct NS_STACK_CLASS A
{
  
  
  A();

  int i;
};

void* Foo()
{
  return new A();
}
