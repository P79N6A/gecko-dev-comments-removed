




#include "MacIOSurfaceImage.h"
#include "mozilla/layers/TextureClientOGL.h"

using namespace mozilla::layers;

TextureClient*
MacIOSurfaceImage::GetTextureClient()
{
  if (!mTextureClient) {
    RefPtr<MacIOSurfaceTextureClientOGL> buffer =
      new MacIOSurfaceTextureClientOGL(TEXTURE_FLAGS_DEFAULT);
    buffer->InitWith(mSurface);
    mTextureClient = buffer;
  }
  return mTextureClient;
}
