




#ifndef MOZILLA_LAYERS_TEXTURECHILD_H
#define MOZILLA_LAYERS_TEXTURECHILD_H

#include "mozilla/layers/PTextureChild.h"
#include "CompositableClient.h"

namespace mozilla {
namespace layers {

class TextureClient;

class TextureChild : public PTextureChild
{
public:
  TextureChild()
  : mTextureClient(nullptr)
  {
    MOZ_COUNT_CTOR(TextureClient);
  }
  ~TextureChild()
  {
    MOZ_COUNT_DTOR(TextureClient);
  }

  void SetClient(TextureClient* aTextureClient)
  {
    mTextureClient = aTextureClient;
  }

  CompositableClient* GetCompositableClient();
  TextureClient* GetTextureClient() const
  {
    return mTextureClient;
  }

  void Destroy();

private:
  TextureClient* mTextureClient;
};

} 
} 

#endif
