




#include "TestDrawTargetD2D.h"

using namespace mozilla::gfx;
TestDrawTargetD2D::TestDrawTargetD2D()
{
  ::D3D10CreateDevice1(NULL,
                       D3D10_DRIVER_TYPE_HARDWARE,
                       NULL,
                       D3D10_CREATE_DEVICE_BGRA_SUPPORT |
                       D3D10_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
                       D3D10_FEATURE_LEVEL_10_0,
                       D3D10_1_SDK_VERSION,
                       byRef(mDevice));

  Factory::SetDirect3D10Device(mDevice);

  mDT = Factory::CreateDrawTarget(BACKEND_DIRECT2D, IntSize(DT_WIDTH, DT_HEIGHT), FORMAT_B8G8R8A8);
}
