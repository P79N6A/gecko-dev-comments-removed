




#include "mozilla/layers/CompositableClient.h"
#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/TextureClientOGL.h"
#include "mozilla/layers/LayerTransactionChild.h"
#include "mozilla/layers/CompositableForwarder.h"

namespace mozilla {
namespace layers {

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

TemporaryRef<TextureClient>
CompositableClient::CreateTextureClient(TextureClientType aTextureClientType)
{
  MOZ_ASSERT(GetForwarder(), "Can't create a texture client if the compositable is not connected to the compositor.");
  LayersBackend parentBackend = GetForwarder()->GetCompositorBackendType();
  RefPtr<TextureClient> result;

  switch (aTextureClientType) {
  case TEXTURE_SHARED_GL:
    if (parentBackend == LAYERS_OPENGL) {
      result = new TextureClientSharedOGL(GetForwarder(), GetTextureInfo());
    }
     break;
  case TEXTURE_SHARED_GL_EXTERNAL:
    if (parentBackend == LAYERS_OPENGL) {
      result = new TextureClientSharedOGLExternal(GetForwarder(), GetTextureInfo());
    }
    break;
  case TEXTURE_STREAM_GL:
    if (parentBackend == LAYERS_OPENGL) {
      result = new TextureClientStreamOGL(GetForwarder(), GetTextureInfo());
    }
    break;
  case TEXTURE_YCBCR:
    result = new TextureClientShmemYCbCr(GetForwarder(), GetTextureInfo());
    break;
  case TEXTURE_CONTENT:
     
  case TEXTURE_SHMEM:
    if (parentBackend == LAYERS_OPENGL) {
      result = new TextureClientShmem(GetForwarder(), GetTextureInfo());
    }
    break;
  default:
    MOZ_ASSERT(false, "Unhandled texture client type");
  }

  MOZ_ASSERT(result, "Failed to create TextureClient");
  MOZ_ASSERT(result->SupportsType(aTextureClientType),
             "Created the wrong texture client?");
  result->SetFlags(GetTextureInfo().mTextureFlags);

  return result.forget();
}

} 
} 
