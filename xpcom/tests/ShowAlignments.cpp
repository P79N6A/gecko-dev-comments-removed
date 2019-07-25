






































#include "nsMemory.h"
#include <stdio.h>

struct S {
  double d;
  char c;
  short s;
};

int main()
{
  static const char str[] =
    "Type %s has size %u and alignment requirement %u\n";
  #define SHOW_TYPE(t_) \
    printf(str, #t_, unsigned(sizeof(t_)), unsigned(NS_ALIGNMENT_OF(t_)))

  SHOW_TYPE(char);
  SHOW_TYPE(unsigned short);
  SHOW_TYPE(int);
  SHOW_TYPE(long);
  SHOW_TYPE(PRUint8);
  SHOW_TYPE(PRInt16);
  SHOW_TYPE(PRUint32);
  SHOW_TYPE(PRFloat64);
  SHOW_TYPE(void*);
  SHOW_TYPE(double);
  SHOW_TYPE(short[7]);
  SHOW_TYPE(S);
  SHOW_TYPE(double S::*);

  return 0;
}
