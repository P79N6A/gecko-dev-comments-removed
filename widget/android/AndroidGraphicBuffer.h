




































#ifndef AndroidGraphicBuffer_h_
#define AndroidGraphicBuffer_h_

#include "gfxASurface.h"
#include "nsRect.h"

namespace mozilla {










class AndroidGraphicBuffer
{
public:
  enum {
    UsageSoftwareRead = 1,
    UsageSoftwareWrite = 1 << 1,
    UsageTexture = 1 << 2,
    UsageTarget = 1 << 3,
    Usage2D = 1 << 4
  };

  AndroidGraphicBuffer(PRUint32 width, PRUint32 height, PRUint32 usage, gfxASurface::gfxImageFormat format);
  virtual ~AndroidGraphicBuffer();

  bool Lock(PRUint32 usage, unsigned char **bits);
  bool Lock(PRUint32 usage, const nsIntRect& rect, unsigned char **bits);
  bool Unlock();
  bool Reallocate(PRUint32 aWidth, PRUint32 aHeight, gfxASurface::gfxImageFormat aFormat);

  PRUint32 Width() { return mWidth; }
  PRUint32 Height() { return mHeight; }

  bool Bind();

private:
  PRUint32 mWidth;
  PRUint32 mHeight;
  PRUint32 mUsage;
  gfxASurface::gfxImageFormat mFormat;

  bool EnsureInitialized();
  bool EnsureEGLImage();

  void DestroyBuffer();
  bool EnsureBufferCreated();

  PRUint32 GetAndroidUsage(PRUint32 aUsage);
  PRUint32 GetAndroidFormat(gfxASurface::gfxImageFormat aFormat);

  void *mHandle;
  void *mEGLImage;
};

} 
#endif
