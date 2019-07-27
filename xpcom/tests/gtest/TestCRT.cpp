





#include "nsCRT.h"
#include "nsString.h"
#include "plstr.h"
#include <stdlib.h>
#include "gtest/gtest.h"

namespace TestCRT {



int sign(int val) {
    if (val == 0)
	return 0;
    else {
	if (val > 0)
	    return 1;
	else
	    return -1;
    }
}





static void Check(const char* s1, const char* s2, int n)
{
  int clib = PL_strcmp(s1, s2);

  int clib_n = PL_strncmp(s1, s2, n);

  nsAutoString t1,t2; 
  t1.AssignWithConversion(s1);
  t2.AssignWithConversion(s2);
  const char16_t* us1 = t1.get();
  const char16_t* us2 = t2.get();

  int u2 = nsCRT::strcmp(us1, us2);

  int u2_n = nsCRT::strncmp(us1, us2, n);

  EXPECT_EQ(sign(clib), sign(u2));
  EXPECT_EQ(sign(clib), sign(u2_n));
  EXPECT_EQ(sign(clib), sign(clib_n));
}

struct Test {
  const char* s1;
  const char* s2;
  int n;
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

TEST(CRT, main)
{
  TestCRT::Test* tp = tests;
  for (int i = 0; i < NUM_TESTS; i++, tp++) {
    Check(tp->s1, tp->s2, tp->n);
  }
}

}
