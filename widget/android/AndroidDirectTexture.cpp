




































#include <unistd.h>
#include <errno.h>
#include "AndroidDirectTexture.h"

typedef gfxASurface::gfxImageFormat gfxImageFormat;

namespace mozilla {

AndroidDirectTexture::AndroidDirectTexture(PRUint32 width, PRUint32 height, PRUint32 usage,
                                           gfxImageFormat format) :
    mLock("AndroidDirectTexture.mLock")
  , mNeedFlip(false)
  , mWidth(width)
  , mHeight(height)
  , mFormat(format)
  , mPendingReallocBuffer(NULL)
{
  mFrontBuffer = new AndroidGraphicBuffer(width, height, usage, format);
  mBackBuffer = new AndroidGraphicBuffer(width, height, usage, format);
}

AndroidDirectTexture::~AndroidDirectTexture()
{
  if (mFrontBuffer) {
    delete mFrontBuffer;
    mFrontBuffer = NULL;
  }

  if (mBackBuffer) {
    delete mBackBuffer;
    mBackBuffer = NULL;
  }
}

void
AndroidDirectTexture::ReallocPendingBuffer()
{
  
  
  
  
  
  if (mPendingReallocBuffer == mBackBuffer) {
    mBackBuffer->Reallocate(mWidth, mHeight, mFormat);
    mPendingReallocBuffer = NULL;
  }
}

bool
AndroidDirectTexture::Lock(PRUint32 aUsage, unsigned char **bits)
{
  nsIntRect rect(0, 0, mWidth, mHeight);
  return Lock(aUsage, rect, bits);
}

bool
AndroidDirectTexture::Lock(PRUint32 aUsage, const nsIntRect& aRect, unsigned char **bits)
{
  mLock.Lock();
  ReallocPendingBuffer();

  int result;
  while ((result = mBackBuffer->Lock(aUsage, aRect, bits)) == -EBUSY) {
    usleep(1000); 
  }

  return result == 0;
}

bool
AndroidDirectTexture::Unlock(bool aFlip)
{
  if (aFlip) {
    mNeedFlip = true;
  }

  bool result = mBackBuffer->Unlock();
  mLock.Unlock();

  return result;
}

bool
AndroidDirectTexture::Reallocate(PRUint32 aWidth, PRUint32 aHeight) {
  return Reallocate(aWidth, aHeight, mFormat);
}

bool
AndroidDirectTexture::Reallocate(PRUint32 aWidth, PRUint32 aHeight, gfxASurface::gfxImageFormat aFormat)
{
  MutexAutoLock lock(mLock);

  
  
  
  bool result = mBackBuffer->Reallocate(aWidth, aHeight, aFormat);
  if (result) {
    mPendingReallocBuffer = mFrontBuffer;

    mWidth = aWidth;
    mHeight = aHeight;
  }

  return result;
}

bool
AndroidDirectTexture::Bind()
{
  MutexAutoLock lock(mLock);

  if (mNeedFlip) {
    AndroidGraphicBuffer* tmp = mBackBuffer;
    mBackBuffer = mFrontBuffer;
    mFrontBuffer = tmp;
    mNeedFlip = false;
  }

  return mFrontBuffer->Bind();
}

} 
