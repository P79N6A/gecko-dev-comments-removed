




#pragma once

#include "TestDrawTargetBase.h"

#include <d3d10_1.h>

class TestDrawTargetD2D : public TestDrawTargetBase
{
public:
  TestDrawTargetD2D();

private:
  mozilla::RefPtr<ID3D10Device1> mDevice;
};
