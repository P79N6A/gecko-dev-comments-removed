



#include <stdint.h>
#include "prtypes.h"





static void
ClearNSPRIntTypes(PRInt8 *a, PRInt16 *b, PRInt32 *c, PRInt64 *d)
{
  *a = 0; *b = 0; *c = 0; *d = 0;
}

static void
ClearStdIntTypes(int8_t *w, int16_t *x, int32_t *y, int64_t *z)
{
  *w = 0; *x = 0; *y = 0; *z = 0;
}

int
main()
{
  PRInt8 a; PRInt16 b; PRInt32 c; PRInt64 d;
  int8_t w; int16_t x; int32_t y; int64_t z;

  ClearNSPRIntTypes(&w, &x, &y, &z);
  ClearStdIntTypes(&a, &b, &c, &d);
  return 0;
}
