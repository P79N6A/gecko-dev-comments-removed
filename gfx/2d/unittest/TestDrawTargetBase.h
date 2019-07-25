




































#pragma once

#include "2D.h"
#include "TestBase.h"

#define DT_WIDTH 500
#define DT_HEIGHT 500





class TestDrawTargetBase : public TestBase
{
public:
  void Initialized();
  void FillCompletely();
  void FillRect();

protected:
  TestDrawTargetBase();

  void RefreshSnapshot();

  void VerifyAllPixels(const mozilla::gfx::Color &aColor);
  void VerifyPixel(const mozilla::gfx::IntPoint &aPoint,
                   mozilla::gfx::Color &aColor);

  uint32_t RGBAPixelFromColor(const mozilla::gfx::Color &aColor);

  mozilla::RefPtr<mozilla::gfx::DrawTarget> mDT;
  mozilla::RefPtr<mozilla::gfx::DataSourceSurface> mDataSnapshot;
};
