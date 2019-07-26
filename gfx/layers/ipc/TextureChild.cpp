




#include "mozilla/layers/TextureChild.h"
#include "mozilla/layers/TextureClient.h"

namespace mozilla {
namespace layers {

CompositableClient*
TextureChild::GetCompositableClient()
{
  return static_cast<CompositableChild*>(Manager())->GetCompositableClient();
}

void
TextureChild::Destroy()
{
  if (mTextureClient) {
  	mTextureClient->SetIPDLActor(nullptr);
  	mTextureClient = nullptr;
  }
  Send__delete__(this);
}

} 
} 