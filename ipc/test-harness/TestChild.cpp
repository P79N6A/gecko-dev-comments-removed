




































#include "TestChild.h"

using mozilla::test::TestChild;


TestChild::TestChild()
{
}

TestChild::~TestChild()
{
}

#if 1


bool TestChild::RecvHello()
{
  puts("[TestChild] Hello, ");
  SendWorld();
  return true;
}


#elif 0


bool TestChild::RecvPing()
{
  return SendPong(42);
}

bool TestChild::RecvPong(const int& status)
{
  return false;
}

bool TestChild::RecvTellValue(
            const String& key,
            const String& val)
{
  return false;
}

bool TestChild::RecvTellValues(
            const StringArray& keys,
            const StringArray& vals)
{
  return false;
}
#endif
