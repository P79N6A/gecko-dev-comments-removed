




#include "mozilla/layers/TextureHost.h"
#include "mozilla/layers/LayersSurfaces.h"
#include "LayersLogging.h"
#include "nsPrintfCString.h"

namespace mozilla {
namespace layers {


TemporaryRef<TextureHost> CreateTextureHostOGL(SurfaceDescriptorType aDescriptorType,
                                               uint32_t aTextureHostFlags,
                                               uint32_t aTextureFlags);

TemporaryRef<TextureHost> CreateTextureHostD3D9(SurfaceDescriptorType aDescriptorType,
                                                uint32_t aTextureHostFlags,
                                                uint32_t aTextureFlags)
{
  NS_RUNTIMEABORT("not implemented");
  return nullptr;
}

 TemporaryRef<TextureHost>
TextureHost::CreateTextureHost(SurfaceDescriptorType aDescriptorType,
                               uint32_t aTextureHostFlags,
                               uint32_t aTextureFlags)
{
  switch (Compositor::GetBackend()) {
    case LAYERS_OPENGL : return CreateTextureHostOGL(aDescriptorType,
                                                     aTextureHostFlags,
                                                     aTextureFlags);
    case LAYERS_D3D9 : return CreateTextureHostD3D9(aDescriptorType,
                                                    aTextureHostFlags,
                                                    aTextureFlags);
    default : return nullptr;
  }
}


TextureHost::TextureHost()
  : mFlags(0)
  , mBuffer(nullptr)
  , mFormat(gfx::FORMAT_UNKNOWN)
  , mDeAllocator(nullptr)
{
  MOZ_COUNT_CTOR(TextureHost);
}

TextureHost::~TextureHost()
{
  if (mBuffer) {
    if (mDeAllocator) {
      mDeAllocator->DestroySharedSurface(mBuffer);
    } else {
      MOZ_ASSERT(mBuffer->type() == SurfaceDescriptor::Tnull_t);
    }
    delete mBuffer;
  }
  MOZ_COUNT_DTOR(TextureHost);
}

void
TextureHost::Update(const SurfaceDescriptor& aImage,
                    nsIntRegion* aRegion)
{
  UpdateImpl(aImage, aRegion);
}

void
TextureHost::SwapTextures(const SurfaceDescriptor& aImage,
                          SurfaceDescriptor* aResult,
                          nsIntRegion* aRegion)
{
  SwapTexturesImpl(aImage, aRegion);

  MOZ_ASSERT(mBuffer, "trying to swap a non-buffered texture host?");
  if (aResult) {
    *aResult = *mBuffer;
  }
  *mBuffer = aImage;
}

#ifdef MOZ_LAYERS_HAVE_LOG
void
TextureSource::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("UnknownTextureSource (0x%p)", this);
}

void
TextureHost::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("%s (0x%p)", Name(), this);
  AppendToString(aTo, GetSize(), " [size=", "]");
  AppendToString(aTo, GetFormat(), " [format=", "]");
  AppendToString(aTo, mFlags, " [flags=", "]");
}
#endif 


} 
} 
