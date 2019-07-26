




#pragma once

#include "TestBase.h"

class TestScaling : public TestBase
{
public:
  TestScaling();

  void BasicHalfScale();
  void DoubleHalfScale();
  void UnevenHalfScale();
  void OddStrideHalfScale();
  void VerticalHalfScale();
  void HorizontalHalfScale();
  void MixedHalfScale();
};
