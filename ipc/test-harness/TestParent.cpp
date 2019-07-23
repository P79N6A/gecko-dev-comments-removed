




































#include "TestParent.h"

using mozilla::test::TestParent;


TestParent::TestParent()
{
}

TestParent::~TestParent()
{
}


void
TestParent::DoStuff()
{
#if 1
  puts("[TestParent] in DoStuff()");
  SendHello();
#elif 0
  puts("[TestParent] pinging child ...");
  SendPing();
#endif
}


#if 1


bool TestParent::RecvWorld()
{
  puts("[TestParent] world!");
  return true;
}


#elif 0


bool TestParent::RecvPing()
{
  return false;
}

bool TestParent::RecvPong(const int& status)
{
  printf("[TestParent] child replied to ping with status code %d\n", status);
  return true;
}

bool TestParent::RecvGetValue(const String& key)
{
  return false;
}

bool TestParent::RecvGetValues(const StringArray& keys)
{
  return false;
}

bool TestParent::RecvSetValue(
            const String& key,
            const String& val,
            bool* ok)
{
  return false;
}

#endif
