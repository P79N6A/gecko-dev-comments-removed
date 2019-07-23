




































#include "nsCRT.h"
#include "nsString.h"
#include "plstr.h"
#include <stdlib.h>



PRIntn sign(PRIntn val) {
    if (val == 0)
	return 0;
    else {
	if (val > 0)
	    return 1;
	else
	    return -1;
    }
}





static void Check(const char* s1, const char* s2, PRIntn n)
{
  PRIntn clib = PL_strcmp(s1, s2);
  PRIntn clib_n = PL_strncmp(s1, s2, n);
  PRIntn clib_case = PL_strcasecmp(s1, s2);
  PRIntn clib_case_n = PL_strncasecmp(s1, s2, n);

  nsAutoString t1,t2; 
  t1.AssignWithConversion(s1);
  t2.AssignWithConversion(s2);
  const PRUnichar* us1 = t1.get();
  const PRUnichar* us2 = t2.get();

  PRIntn u2 = nsCRT::strcmp(us1, us2);
  PRIntn u2_n = nsCRT::strncmp(us1, us2, n);

  NS_ASSERTION(sign(clib) == sign(u2), "strcmp");
  NS_ASSERTION(sign(clib_n) == sign(u2_n), "strncmp");
}

struct Test {
  const char* s1;
  const char* s2;
  PRIntn n;
};

static Test tests[] = {
  { "foo", "foo", 3 },
  { "foo", "fo", 3 },

  { "foo", "bar", 3 },
  { "foo", "ba", 3 },

  { "foo", "zap", 3 },
  { "foo", "za", 3 },

  { "bar", "foo", 3 },
  { "bar", "fo", 3 },

  { "bar", "foo", 3 },
  { "bar", "fo", 3 },
};
#define NUM_TESTS int((sizeof(tests) / sizeof(tests[0])))

int main()
{
  Test* tp = tests;
  for (int i = 0; i < NUM_TESTS; i++, tp++) {
    Check(tp->s1, tp->s2, tp->n);
  }

  return 0;
}
