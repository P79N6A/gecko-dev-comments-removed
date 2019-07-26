






#include "mozilla/DebugOnly.h"

#include "mozilla/layers/PGrallocBufferChild.h"
#include "mozilla/layers/PGrallocBufferParent.h"
#include "mozilla/layers/PLayerTransactionChild.h"
#include "mozilla/layers/ShadowLayers.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/unused.h"
#include "nsXULAppAPI.h"

#include "ShadowLayerUtilsGralloc.h"

#include "nsIMemoryReporter.h"

#include "gfxImageSurface.h"
#include "gfxPlatform.h"

#include "GeckoProfiler.h"

#include "cutils/properties.h"

using namespace android;
using namespace base;
using namespace mozilla::layers;
using namespace mozilla::gl;

namespace IPC {

void
ParamTraits<MagicGrallocBufferHandle>::Write(Message* aMsg,
                                             const paramType& aParam)
{
  Flattenable *flattenable = aParam.mGraphicBuffer.get();
  size_t nbytes = flattenable->getFlattenedSize();
  size_t nfds = flattenable->getFdCount();

  char data[nbytes];
  int fds[nfds];
  flattenable->flatten(data, nbytes, fds, nfds);

  aMsg->WriteSize(nbytes);
  aMsg->WriteSize(nfds);

  aMsg->WriteBytes(data, nbytes);
  for (size_t n = 0; n < nfds; ++n) {
    
    
    
    aMsg->WriteFileDescriptor(FileDescriptor(fds[n], false));
  }
}

bool
ParamTraits<MagicGrallocBufferHandle>::Read(const Message* aMsg,
                                            void** aIter, paramType* aResult)
{
  size_t nbytes;
  size_t nfds;
  const char* data;

  if (!aMsg->ReadSize(aIter, &nbytes) ||
      !aMsg->ReadSize(aIter, &nfds) ||
      !aMsg->ReadBytes(aIter, &data, nbytes)) {
    return false;
  }

  int fds[nfds];

  for (size_t n = 0; n < nfds; ++n) {
    FileDescriptor fd;
    if (!aMsg->ReadFileDescriptor(aIter, &fd)) {
      return false;
    }
    
    
    
    
    
    bool sameProcess = (XRE_GetProcessType() == GeckoProcessType_Default);
    int dupFd = sameProcess ? dup(fd.fd) : fd.fd;
    fds[n] = dupFd;
  }

  sp<GraphicBuffer> buffer(new GraphicBuffer());
  Flattenable *flattenable = buffer.get();

  if (NO_ERROR == flattenable->unflatten(data, nbytes, fds, nfds)) {
    aResult->mGraphicBuffer = buffer;
    return true;
  }
  return false;
}

} 

namespace mozilla {
namespace layers {

MagicGrallocBufferHandle::MagicGrallocBufferHandle(const sp<GraphicBuffer>& aGraphicBuffer)
  : mGraphicBuffer(aGraphicBuffer)
{
}




static gfxASurface::gfxImageFormat
ImageFormatForPixelFormat(android::PixelFormat aFormat)
{
  switch (aFormat) {
  case PIXEL_FORMAT_RGBA_8888:
    return gfxASurface::ImageFormatARGB32;
  case PIXEL_FORMAT_RGBX_8888:
    return gfxASurface::ImageFormatRGB24;
  case PIXEL_FORMAT_RGB_565:
    return gfxASurface::ImageFormatRGB16_565;
  case PIXEL_FORMAT_A_8:
    return gfxASurface::ImageFormatA8;
  default:
    MOZ_CRASH("Unknown gralloc pixel format");
  }
  return gfxASurface::ImageFormatARGB32;
}

static android::PixelFormat
PixelFormatForImageFormat(gfxASurface::gfxImageFormat aFormat)
{
  switch (aFormat) {
  case gfxASurface::ImageFormatARGB32:
    return android::PIXEL_FORMAT_RGBA_8888;
  case gfxASurface::ImageFormatRGB24:
    return android::PIXEL_FORMAT_RGBX_8888;
  case gfxASurface::ImageFormatRGB16_565:
    return android::PIXEL_FORMAT_RGB_565;
  case gfxASurface::ImageFormatA8:
    return android::PIXEL_FORMAT_A_8;
  default:
    MOZ_CRASH("Unknown gralloc pixel format");
  }
  return gfxASurface::ImageFormatARGB32;
}

static size_t
BytesPerPixelForPixelFormat(android::PixelFormat aFormat)
{
  switch (aFormat) {
  case PIXEL_FORMAT_RGBA_8888:
  case PIXEL_FORMAT_RGBX_8888:
  case PIXEL_FORMAT_BGRA_8888:
    return 4;
  case PIXEL_FORMAT_RGB_888:
    return 3;
  case PIXEL_FORMAT_RGB_565:
  case PIXEL_FORMAT_RGBA_5551:
  case PIXEL_FORMAT_RGBA_4444:
    return 2;
  case PIXEL_FORMAT_A_8:
    return 1;
  default:
    return 0;
  }
  return 0;
}

static android::PixelFormat
PixelFormatForContentType(gfxASurface::gfxContentType aContentType)
{
  return PixelFormatForImageFormat(
    gfxPlatform::GetPlatform()->OptimalFormatForContent(aContentType));
}

static gfxASurface::gfxContentType
ContentTypeFromPixelFormat(android::PixelFormat aFormat)
{
  return gfxASurface::ContentFromFormat(ImageFormatForPixelFormat(aFormat));
}

static size_t sCurrentAlloc;
static int64_t GetGrallocSize() { return sCurrentAlloc; }

NS_MEMORY_REPORTER_IMPLEMENT(GrallocBufferActor,
  "gralloc",
  KIND_OTHER,
  UNITS_BYTES,
  GetGrallocSize,
  "Special RAM that can be shared between processes and directly "
  "accessed by both the CPU and GPU.  Gralloc memory is usually a "
  "relatively precious resource, with much less available than generic "
  "RAM.  When it's exhausted, graphics performance can suffer. "
  "This value can be incorrect because of race conditions.");

GrallocBufferActor::GrallocBufferActor()
: mAllocBytes(0)
, mDeprecatedTextureHost(nullptr)
{
  static bool registered;
  if (!registered) {
    
    
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(GrallocBufferActor));
    registered = true;
  }
}

GrallocBufferActor::~GrallocBufferActor()
{
  if (mAllocBytes > 0) {
    sCurrentAlloc -= mAllocBytes;
  }
}

 PGrallocBufferParent*
GrallocBufferActor::Create(const gfxIntSize& aSize,
                           const uint32_t& aFormat,
                           const uint32_t& aUsage,
                           MaybeMagicGrallocBufferHandle* aOutHandle)
{
  PROFILER_LABEL("GrallocBufferActor", "Create");
  GrallocBufferActor* actor = new GrallocBufferActor();
  *aOutHandle = null_t();
  uint32_t format = aFormat;
  uint32_t usage = aUsage;

  if (format == 0 || usage == 0) {
    printf_stderr("GrallocBufferActor::Create -- format and usage must be non-zero");
    return actor;
  }

  sp<GraphicBuffer> buffer(new GraphicBuffer(aSize.width, aSize.height, format, usage));
  if (buffer->initCheck() != OK)
    return actor;

  size_t bpp = BytesPerPixelForPixelFormat(format);
  actor->mAllocBytes = aSize.width * aSize.height * bpp;
  sCurrentAlloc += actor->mAllocBytes;

  actor->mGraphicBuffer = buffer;
  *aOutHandle = MagicGrallocBufferHandle(buffer);

  return actor;
}


void GrallocBufferActor::ActorDestroy(ActorDestroyReason)
{
  if (mDeprecatedTextureHost) {
    mDeprecatedTextureHost->ForgetBuffer();
  }
}


void GrallocBufferActor::SetDeprecatedTextureHost(DeprecatedTextureHost* aDeprecatedTextureHost)
{
  mDeprecatedTextureHost = aDeprecatedTextureHost;
}

 already_AddRefed<TextureImage>
LayerManagerComposite::OpenDescriptorForDirectTexturing(GLContext* aGL,
                                                        const SurfaceDescriptor& aDescriptor,
                                                        GLenum aWrapMode)
{
  PROFILER_LABEL("LayerManagerComposite", "OpenDescriptorForDirectTexturing");
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aDescriptor.type()) {
    return nullptr;
  }
  sp<GraphicBuffer> buffer = GrallocBufferActor::GetFrom(aDescriptor);
  return aGL->CreateDirectTextureImage(buffer.get(), aWrapMode);
}

 bool
LayerManagerComposite::SupportsDirectTexturing()
{
  return true;
}

 void
LayerManagerComposite::PlatformSyncBeforeReplyUpdate()
{
  
}

bool
ISurfaceAllocator::PlatformDestroySharedSurface(SurfaceDescriptor* aSurface)
{
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aSurface->type()) {
    return false;
  }

  
  
  PGrallocBufferParent* gbp =
    aSurface->get_SurfaceDescriptorGralloc().bufferParent();
  if (gbp) {
    unused << PGrallocBufferParent::Send__delete__(gbp);
  } else {
    PGrallocBufferChild* gbc =
      aSurface->get_SurfaceDescriptorGralloc().bufferChild();
    unused << PGrallocBufferChild::Send__delete__(gbc);
  }

  *aSurface = SurfaceDescriptor();
  return true;
}




 PGrallocBufferChild*
GrallocBufferActor::Create()
{
  return new GrallocBufferActor();
}

void
GrallocBufferActor::InitFromHandle(const MagicGrallocBufferHandle& aHandle)
{
  MOZ_ASSERT(!mGraphicBuffer.get());
  MOZ_ASSERT(aHandle.mGraphicBuffer.get());

  mGraphicBuffer = aHandle.mGraphicBuffer;
}

PGrallocBufferChild*
ShadowLayerForwarder::AllocGrallocBuffer(const gfxIntSize& aSize,
                                         uint32_t aFormat,
                                         uint32_t aUsage,
                                         MaybeMagicGrallocBufferHandle* aHandle)
{
  return mShadowManager->SendPGrallocBufferConstructor(aSize, aFormat, aUsage, aHandle);
}

bool
ISurfaceAllocator::PlatformAllocSurfaceDescriptor(const gfxIntSize& aSize,
                                                  gfxASurface::gfxContentType aContent,
                                                  uint32_t aCaps,
                                                  SurfaceDescriptor* aBuffer)
{

  
  
#if ANDROID_VERSION <= 15
  static bool checkedDevice = false;
  static bool disableGralloc = false;

  if (!checkedDevice) {
    char propValue[PROPERTY_VALUE_MAX];
    property_get("ro.product.device", propValue, "None");

    if (strcmp("crespo",propValue) == 0) {
      NS_WARNING("Nexus S has issues with gralloc, falling back to shmem");
      disableGralloc = true;
    } else if (strcmp("peak", propValue) == 0) {
      NS_WARNING("Geeksphone Peak has issues with gralloc, falling back to shmem");
      disableGralloc = true;
    }

    checkedDevice = true;
  }

  if (disableGralloc) {
    return false;
  }
#endif

  




  gfxIntSize allocSize = aSize;
  allocSize.width = std::max(aSize.width, 32);
  allocSize.height = std::max(aSize.height, 32);

  PROFILER_LABEL("ShadowLayerForwarder", "PlatformAllocSurfaceDescriptor");
  
  
  MaybeMagicGrallocBufferHandle handle;
  PGrallocBufferChild* gc;
  bool defaultRBSwap;

  if (aCaps & USING_GL_RENDERING_ONLY) {
    gc = AllocGrallocBuffer(allocSize,
                            PixelFormatForContentType(aContent),
                            GraphicBuffer::USAGE_HW_RENDER |
                            GraphicBuffer::USAGE_HW_TEXTURE,
                            &handle);
    
    
    defaultRBSwap = false;
  } else {
    gc = AllocGrallocBuffer(allocSize,
                            PixelFormatForContentType(aContent),
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN |
                            GraphicBuffer::USAGE_HW_TEXTURE,
                            &handle);
    
    
    
    defaultRBSwap = true;
  }

  if (!gc) {
    NS_ERROR("GrallocBufferConstructor failed by returned null");
    return false;
  } else if (handle.Tnull_t == handle.type()) {
    NS_ERROR("GrallocBufferConstructor failed by returning handle with type Tnull_t");
    PGrallocBufferChild::Send__delete__(gc);
    return false;
  }

  GrallocBufferActor* gba = static_cast<GrallocBufferActor*>(gc);
  gba->InitFromHandle(handle.get_MagicGrallocBufferHandle());

  *aBuffer = SurfaceDescriptorGralloc(nullptr, gc, allocSize,
                                       false,
                                      defaultRBSwap);
  return true;
}




 sp<GraphicBuffer>
GrallocBufferActor::GetFrom(const SurfaceDescriptorGralloc& aDescriptor)
{
  GrallocBufferActor* gba = nullptr;
  if (PGrallocBufferChild* child = aDescriptor.bufferChild()) {
    gba = static_cast<GrallocBufferActor*>(child);
  } else if (PGrallocBufferParent* parent = aDescriptor.bufferParent()) {
    gba = static_cast<GrallocBufferActor*>(parent);
  }
  return gba->mGraphicBuffer;
}


 already_AddRefed<gfxASurface>
ShadowLayerForwarder::PlatformOpenDescriptor(OpenMode aMode,
                                             const SurfaceDescriptor& aSurface)
{
  PROFILER_LABEL("ShadowLayerForwarder", "PlatformOpenDescriptor");
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aSurface.type()) {
    return nullptr;
  }

  sp<GraphicBuffer> buffer =
    GrallocBufferActor::GetFrom(aSurface.get_SurfaceDescriptorGralloc());
  uint32_t usage = GRALLOC_USAGE_SW_READ_OFTEN;
  if (OPEN_READ_WRITE == aMode) {
    usage |= GRALLOC_USAGE_SW_WRITE_OFTEN;
  }
  void *vaddr;
  DebugOnly<status_t> status = buffer->lock(usage, &vaddr);
  
  MOZ_ASSERT(status == OK);

  gfxIntSize size = aSurface.get_SurfaceDescriptorGralloc().size();
  gfxImageFormat format = ImageFormatForPixelFormat(buffer->getPixelFormat());
  long pixelStride = buffer->getStride();
  long byteStride = pixelStride * gfxASurface::BytePerPixelFromFormat(format);

  nsRefPtr<gfxASurface> surf =
    new gfxImageSurface((unsigned char*)vaddr, size, byteStride, format);
  return surf->CairoStatus() ? nullptr : surf.forget();
}

 bool
ShadowLayerForwarder::PlatformGetDescriptorSurfaceContentType(
  const SurfaceDescriptor& aDescriptor, OpenMode aMode,
  gfxContentType* aContent,
  gfxASurface** aSurface)
{
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aDescriptor.type()) {
    return false;
  }

  sp<GraphicBuffer> buffer =
    GrallocBufferActor::GetFrom(aDescriptor.get_SurfaceDescriptorGralloc());
  *aContent = ContentTypeFromPixelFormat(buffer->getPixelFormat());
  return true;
}

 bool
ShadowLayerForwarder::PlatformGetDescriptorSurfaceSize(
  const SurfaceDescriptor& aDescriptor, OpenMode aMode,
  gfxIntSize* aSize,
  gfxASurface** aSurface)
{
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aDescriptor.type()) {
    return false;
  }

  sp<GraphicBuffer> buffer =
    GrallocBufferActor::GetFrom(aDescriptor.get_SurfaceDescriptorGralloc());
  *aSize = aDescriptor.get_SurfaceDescriptorGralloc().size();
  return true;
}

 bool
ShadowLayerForwarder::PlatformDestroySharedSurface(SurfaceDescriptor* aSurface)
{
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aSurface->type()) {
    return false;
  }

  PGrallocBufferChild* gbp =
    aSurface->get_SurfaceDescriptorGralloc().bufferChild();
  PGrallocBufferChild::Send__delete__(gbp);
  *aSurface = SurfaceDescriptor();
  return true;
}

 bool
ShadowLayerForwarder::PlatformCloseDescriptor(const SurfaceDescriptor& aDescriptor)
{
  PROFILER_LABEL("ShadowLayerForwarder", "PlatformCloseDescriptor");
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aDescriptor.type()) {
    return false;
  }

  sp<GraphicBuffer> buffer = GrallocBufferActor::GetFrom(aDescriptor);
  DebugOnly<status_t> status = buffer->unlock();
  
  MOZ_ASSERT(status == OK);
  return true;
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
  
}

} 
} 
