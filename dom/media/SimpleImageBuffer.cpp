





#include "SimpleImageBuffer.h"
#include "mozilla/NullPtr.h"

namespace mozilla {

void
SimpleImageBuffer::SetImage(const unsigned char* frame, unsigned int size, int width, int height)
{
  mWidth = width;
  mHeight = height;
  if (!mBuffer || (size > mBufferSize)) {
    if (mBuffer) {
      delete[] mBuffer;
      mBuffer = nullptr;
    }
    mBufferSize = size;
    if (size > 0) {
      mBuffer = new unsigned char[size];
    }
  }

  if (mBuffer) {
    if (frame && (size > 0)) {
      memcpy((void *)mBuffer, (const void*)frame, size);
    }
    mSize = size;
  }
}

void
SimpleImageBuffer::Copy(const SimpleImageBuffer* aImage)
{
  if (aImage) {
    SetImage(aImage->mBuffer, aImage->mSize, aImage->mWidth, aImage->mHeight);
  }
}

const unsigned char*
SimpleImageBuffer::GetImage(unsigned int* size) const
{
  if (size) {
    *size = mSize;
  }
  return mBuffer;
}

} 
