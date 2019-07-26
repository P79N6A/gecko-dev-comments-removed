




#ifndef AndroidDirectTexture_h_
#define AndroidDirectTexture_h_

#include "gfxASurface.h"
#include "mozilla/Mutex.h"
#include "AndroidGraphicBuffer.h"

struct nsIntRect;

namespace mozilla {









class AndroidDirectTexture
{
public:
  AndroidDirectTexture(uint32_t width, uint32_t height, uint32_t usage, gfxASurface::gfxImageFormat format);
  virtual ~AndroidDirectTexture();

  bool Lock(uint32_t usage, unsigned char **bits);
  bool Lock(uint32_t usage, const nsIntRect& rect, unsigned char **bits);
  bool Unlock(bool aFlip = true);

  bool Reallocate(uint32_t aWidth, uint32_t aHeight);
  bool Reallocate(uint32_t aWidth, uint32_t aHeight, gfxASurface::gfxImageFormat aFormat);

  uint32_t Width() { return mWidth; }
  uint32_t Height() { return mHeight; }

  bool Bind();

private:
  mozilla::Mutex mLock;
  bool mNeedFlip;

  uint32_t mWidth;
  uint32_t mHeight;
  gfxASurface::gfxImageFormat mFormat;

  AndroidGraphicBuffer* mFrontBuffer;
  AndroidGraphicBuffer* mBackBuffer;

  AndroidGraphicBuffer* mPendingReallocBuffer;
  void ReallocPendingBuffer();
};

} 
#endif 
