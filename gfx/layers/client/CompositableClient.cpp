




#include "mozilla/layers/CompositableClient.h"
#include <stdint.h>                     
#include "gfxPlatform.h"                
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/TextureClientOGL.h"
#include "mozilla/mozalloc.h"           
#include "gfxASurface.h"                
#ifdef XP_WIN
#include "gfxWindowsPlatform.h"         
#include "mozilla/layers/TextureD3D11.h"
#include "mozilla/layers/TextureD3D9.h"
#endif

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

CompositableClient::CompositableClient(CompositableForwarder* aForwarder,
                                       TextureFlags aTextureFlags)
: mCompositableChild(nullptr)
, mForwarder(aForwarder)
, mTextureFlags(aTextureFlags)
{
  MOZ_COUNT_CTOR(CompositableClient);
}

CompositableClient::~CompositableClient()
{
  MOZ_COUNT_DTOR(CompositableClient);
  Destroy();
}

LayersBackend
CompositableClient::GetCompositorBackendType() const
{
  return mForwarder->GetCompositorBackendType();
}

void
CompositableClient::SetIPDLActor(CompositableChild* aChild)
{
  mCompositableChild = aChild;
}

CompositableChild*
CompositableClient::GetIPDLActor() const
{
  return mCompositableChild;
}

bool
CompositableClient::Connect()
{
  if (!GetForwarder() || GetIPDLActor()) {
    return false;
  }
  GetForwarder()->Connect(this);
  return true;
}

void
CompositableClient::Destroy()
{
  if (!mCompositableChild) {
    return;
  }
  mCompositableChild->SetClient(nullptr);
  mCompositableChild->Destroy();
  mCompositableChild = nullptr;
}

uint64_t
CompositableClient::GetAsyncID() const
{
  if (mCompositableChild) {
    return mCompositableChild->GetAsyncID();
  }
  return 0; 
}


void
CompositableChild::Destroy()
{
  Send__delete__(this);
}

TemporaryRef<BufferTextureClient>
CompositableClient::CreateBufferTextureClient(SurfaceFormat aFormat,
                                              TextureFlags aTextureFlags,
                                              gfx::BackendType aMoz2DBackend)
{
  return TextureClient::CreateBufferTextureClient(GetForwarder(), aFormat,
                                                  aTextureFlags | mTextureFlags,
                                                  aMoz2DBackend);
}

TemporaryRef<TextureClient>
CompositableClient::CreateTextureClientForDrawing(SurfaceFormat aFormat,
                                                  TextureFlags aTextureFlags,
                                                  gfx::BackendType aMoz2DBackend,
                                                  const IntSize& aSizeHint)
{
  return TextureClient::CreateTextureClientForDrawing(GetForwarder(), aFormat,
                                                      aTextureFlags | mTextureFlags,
                                                      aMoz2DBackend,
                                                      aSizeHint);
}

bool
CompositableClient::AddTextureClient(TextureClient* aClient)
{
  return aClient->InitIPDLActor(mForwarder);
}

void
CompositableClient::OnTransaction()
{
}

} 
} 
