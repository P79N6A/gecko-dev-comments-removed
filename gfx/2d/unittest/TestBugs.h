




#pragma once

#include "TestBase.h"

class TestBugs : public TestBase
{
public:
  TestBugs();

  void CairoClip918671();
  void PushPopClip950550();
};

