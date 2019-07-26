




#include "mozilla/layers/CompositableClient.h"
#include <stdint.h>                     
#include "gfxPlatform.h"                
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/TextureClientOGL.h"
#include "mozilla/mozalloc.h"           
#include "gfxASurface.h"                
#ifdef XP_WIN
#include "mozilla/layers/TextureD3D9.h"
#include "mozilla/layers/TextureD3D11.h"
#include "gfxWindowsPlatform.h"
#include "gfx2DGlue.h"
#endif

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

CompositableClient::CompositableClient(CompositableForwarder* aForwarder)
: mNextTextureID(1)
, mCompositableChild(nullptr)
, mForwarder(aForwarder)
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

TemporaryRef<DeprecatedTextureClient>
CompositableClient::CreateDeprecatedTextureClient(DeprecatedTextureClientType aDeprecatedTextureClientType,
                                                  gfxContentType aContentType)
{
  MOZ_ASSERT(GetForwarder(), "Can't create a texture client if the compositable is not connected to the compositor.");
  LayersBackend parentBackend = GetForwarder()->GetCompositorBackendType();
  RefPtr<DeprecatedTextureClient> result;

  switch (aDeprecatedTextureClientType) {
  case TEXTURE_SHARED_GL:
    if (parentBackend == LAYERS_OPENGL) {
      result = new DeprecatedTextureClientSharedOGL(GetForwarder(), GetTextureInfo());
    }
     break;
  case TEXTURE_SHARED_GL_EXTERNAL:
    if (parentBackend == LAYERS_OPENGL) {
      result = new DeprecatedTextureClientSharedOGLExternal(GetForwarder(), GetTextureInfo());
    }
    break;
  case TEXTURE_STREAM_GL:
    if (parentBackend == LAYERS_OPENGL) {
      result = new DeprecatedTextureClientStreamOGL(GetForwarder(), GetTextureInfo());
    }
    break;
  case TEXTURE_YCBCR:
    if (parentBackend == LAYERS_OPENGL ||
        parentBackend == LAYERS_D3D9 ||
        parentBackend == LAYERS_D3D11 ||
        parentBackend == LAYERS_BASIC) {
      result = new DeprecatedTextureClientShmemYCbCr(GetForwarder(), GetTextureInfo());
    }
    break;
  case TEXTURE_CONTENT:
#ifdef XP_WIN
    if (parentBackend == LAYERS_D3D11 && gfxWindowsPlatform::GetPlatform()->GetD2DDevice()) {
      result = new DeprecatedTextureClientD3D11(GetForwarder(), GetTextureInfo());
      break;
    }
    if (parentBackend == LAYERS_D3D9 &&
        !GetForwarder()->ForwardsToDifferentProcess()) {
      
      
      
      
      
      
      if (aContentType == GFX_CONTENT_COLOR_ALPHA ||
          !gfxWindowsPlatform::GetPlatform()->GetD3D9Device()) {
        result = new DeprecatedTextureClientDIB(GetForwarder(), GetTextureInfo());
      } else {
        result = new DeprecatedTextureClientD3D9(GetForwarder(), GetTextureInfo());
      }
      break;
    }
#endif
     
  case TEXTURE_SHMEM:
    result = new DeprecatedTextureClientShmem(GetForwarder(), GetTextureInfo());
    break;
  case TEXTURE_FALLBACK:
#ifdef XP_WIN
    if (parentBackend == LAYERS_D3D11 ||
        parentBackend == LAYERS_D3D9) {
      result = new DeprecatedTextureClientShmem(GetForwarder(), GetTextureInfo());
    }
#endif
    break;
  default:
    MOZ_ASSERT(false, "Unhandled texture client type");
  }

  
  
  
  if (!result) {
    return nullptr;
  }

  MOZ_ASSERT(result->SupportsType(aDeprecatedTextureClientType),
             "Created the wrong texture client?");
  result->SetFlags(GetTextureInfo().mTextureFlags);

  return result.forget();
}

TemporaryRef<BufferTextureClient>
CompositableClient::CreateBufferTextureClient(SurfaceFormat aFormat,
                                              TextureFlags aTextureFlags)
{










  if (gfxPlatform::GetPlatform()->PreferMemoryOverShmem()) {
    RefPtr<BufferTextureClient> result = new MemoryTextureClient(this, aFormat, aTextureFlags);
    return result.forget();
  }
  RefPtr<BufferTextureClient> result = new ShmemTextureClient(this, aFormat, aTextureFlags);
  return result.forget();
}

TemporaryRef<TextureClient>
CompositableClient::CreateTextureClientForDrawing(SurfaceFormat aFormat,
                                                  TextureFlags aTextureFlags)
{
  RefPtr<TextureClient> result;

#ifdef XP_WIN
  LayersBackend parentBackend = GetForwarder()->GetCompositorBackendType();
  
  if (parentBackend == LAYERS_D3D11 && gfxWindowsPlatform::GetPlatform()->GetD2DDevice() &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK)) {
    
  }
  if (parentBackend == LAYERS_D3D9 &&
      !GetForwarder()->ForwardsToDifferentProcess() &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK)) {
    
    if (ContentForFormat(aFormat) == GFX_CONTENT_COLOR_ALPHA) {
      
    } else {
      
    }
  }
#endif
  
  if (!result) {
    result = CreateBufferTextureClient(aFormat, aTextureFlags);
  }

  MOZ_ASSERT(!result || result->AsTextureClientDrawTarget(),
             "Not a TextureClientDrawTarget?");
  return result;
}

uint64_t
CompositableClient::NextTextureID()
{
  ++mNextTextureID;
  
  if (mNextTextureID == 0) {
    ++mNextTextureID;
  }

  return mNextTextureID;
}

bool
CompositableClient::AddTextureClient(TextureClient* aClient)
{
  aClient->SetID(NextTextureID());
  return aClient->InitIPDLActor(mForwarder);
}

void
CompositableClient::OnTransaction()
{
}


void
CompositableChild::ActorDestroy(ActorDestroyReason why)
{
  if (mCompositableClient && why == AbnormalShutdown) {
    mCompositableClient->OnActorDestroy();
  }
}
} 
} 
