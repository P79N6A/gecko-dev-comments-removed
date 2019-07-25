




































#include "SharedDIBSurface.h"

#include "cairo.h"

namespace mozilla {
namespace gfx {

static const cairo_user_data_key_t SHAREDDIB_KEY = {0};

static const long kBytesPerPixel = 4;

bool
SharedDIBSurface::Create(HDC adc, PRUint32 aWidth, PRUint32 aHeight,
                         bool aTransparent)
{
  nsresult rv = mSharedDIB.Create(adc, aWidth, aHeight, aTransparent);
  if (NS_FAILED(rv) || !mSharedDIB.IsValid())
    return false;

  InitSurface(aWidth, aHeight, aTransparent);
  return true;
}

bool
SharedDIBSurface::Attach(Handle aHandle, PRUint32 aWidth, PRUint32 aHeight,
                         bool aTransparent)
{
  nsresult rv = mSharedDIB.Attach(aHandle, aWidth, aHeight, aTransparent);
  if (NS_FAILED(rv) || !mSharedDIB.IsValid())
    return false;

  InitSurface(aWidth, aHeight, aTransparent);
  return true;
}

void
SharedDIBSurface::InitSurface(PRUint32 aWidth, PRUint32 aHeight,
                              bool aTransparent)
{
  long stride = long(aWidth * kBytesPerPixel);
  unsigned char* data = reinterpret_cast<unsigned char*>(mSharedDIB.GetBits());

  gfxImageFormat format = aTransparent ? ImageFormatARGB32 : ImageFormatRGB24;

  gfxImageSurface::InitWithData(data, gfxIntSize(aWidth, aHeight),
                                stride, format);

  cairo_surface_set_user_data(mSurface, &SHAREDDIB_KEY, this, NULL);
}

bool
SharedDIBSurface::IsSharedDIBSurface(gfxASurface* aSurface)
{
  return aSurface &&
    aSurface->GetType() == gfxASurface::SurfaceTypeImage &&
    aSurface->GetData(&SHAREDDIB_KEY);
}

} 
} 
