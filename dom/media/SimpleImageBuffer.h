





#ifndef simple_image_buffer_h
#define simple_image_buffer_h

#include "mozilla/NullPtr.h"
#include "nsISupportsImpl.h"

namespace mozilla {

class SimpleImageBuffer {
NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SimpleImageBuffer)
public:
  SimpleImageBuffer() : mBuffer(nullptr), mBufferSize(0),  mSize(0), mWidth(0), mHeight(0) {}
  void SetImage(const unsigned char* frame, unsigned int size, int width, int height);
  void Copy(const SimpleImageBuffer* aImage);
  const unsigned char* GetImage(unsigned int* size) const;
  void GetWidthAndHeight(int* width, int* height) const
  {
    if (width) {
      *width = mWidth;
    }
    if (height) {
      *height = mHeight;
    }
  }

protected:
  ~SimpleImageBuffer()
  {
    delete[] mBuffer;
  }
  const unsigned char* mBuffer;
  unsigned int mBufferSize;
  unsigned int mSize;
  int mWidth;
  int mHeight;

private:
  SimpleImageBuffer(const SimpleImageBuffer& aImage);
  SimpleImageBuffer& operator=(const SimpleImageBuffer& aImage);
};

} 

#endif 
