




































#include "SanityChecks.h"

SanityChecks::SanityChecks()
{
  REGISTER_TEST(SanityChecks, AlwaysPasses);
}

void
SanityChecks::AlwaysPasses()
{
  bool testMustPass = true;

  VERIFY(testMustPass);
}
