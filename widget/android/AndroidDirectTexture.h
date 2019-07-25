




































#ifndef AndroidDirectTexture_h_
#define AndroidDirectTexture_h_

#include "gfxASurface.h"
#include "nsRect.h"
#include "mozilla/Mutex.h"
#include "AndroidGraphicBuffer.h"

namespace mozilla {









class AndroidDirectTexture
{
public:
  AndroidDirectTexture(PRUint32 width, PRUint32 height, PRUint32 usage, gfxASurface::gfxImageFormat format);
  virtual ~AndroidDirectTexture();

  bool Lock(PRUint32 usage, unsigned char **bits);
  bool Lock(PRUint32 usage, const nsIntRect& rect, unsigned char **bits);
  bool Unlock(bool aFlip = true);

  bool Reallocate(PRUint32 aWidth, PRUint32 aHeight);
  bool Reallocate(PRUint32 aWidth, PRUint32 aHeight, gfxASurface::gfxImageFormat aFormat);

  PRUint32 Width() { return mWidth; }
  PRUint32 Height() { return mHeight; }

  bool Bind();

private:
  mozilla::Mutex mLock;
  bool mNeedFlip;

  PRUint32 mWidth;
  PRUint32 mHeight;
  gfxASurface::gfxImageFormat mFormat;

  AndroidGraphicBuffer* mFrontBuffer;
  AndroidGraphicBuffer* mBackBuffer;

  AndroidGraphicBuffer* mPendingReallocBuffer;
  void ReallocPendingBuffer();
};

} 
#endif 
