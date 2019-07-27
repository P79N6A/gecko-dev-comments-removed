










#if defined(__ANDROID__)
#define rand __rand
#include <stdlib.h>
#undef rand

extern int rand(void)
{
  return __rand();
}
#else

int vp8_unused;
#endif
