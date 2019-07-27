




#ifndef AndroidGraphicBuffer_h_
#define AndroidGraphicBuffer_h_

#include "gfxTypes.h"
#include "nsRect.h"

typedef void* EGLImageKHR;
typedef void* EGLClientBuffer;

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

  AndroidGraphicBuffer(uint32_t width, uint32_t height, uint32_t usage, gfxImageFormat format);
  virtual ~AndroidGraphicBuffer();

  int Lock(uint32_t usage, unsigned char **bits);
  int Lock(uint32_t usage, const nsIntRect& rect, unsigned char **bits);
  int Unlock();
  bool Reallocate(uint32_t aWidth, uint32_t aHeight, gfxImageFormat aFormat);

  uint32_t Width() { return mWidth; }
  uint32_t Height() { return mHeight; }

  bool Bind();

  static bool IsBlacklisted();

private:
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mUsage;
  gfxImageFormat mFormat;

  bool EnsureInitialized();
  bool EnsureEGLImage();

  void DestroyBuffer();
  bool EnsureBufferCreated();

  uint32_t GetAndroidUsage(uint32_t aUsage);
  uint32_t GetAndroidFormat(gfxImageFormat aFormat);

  void *mHandle;
  void *mEGLImage;
};

} 
#endif
