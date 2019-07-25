






#include "mozilla/layers/PGrallocBufferChild.h"
#include "mozilla/layers/PGrallocBufferParent.h"
#include "mozilla/layers/PLayersChild.h"
#include "mozilla/layers/ShadowLayers.h"
#include "mozilla/unused.h"
#include "nsXULAppAPI.h"

#include "ShadowLayerUtilsGralloc.h"

#include "gfxImageSurface.h"

using namespace android;
using namespace base;
using namespace mozilla::layers;

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
    MOZ_NOT_REACHED("Unknown gralloc pixel format");
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
    MOZ_NOT_REACHED("Unknown gralloc pixel format");
  }
  return gfxASurface::ImageFormatARGB32;
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

 PGrallocBufferParent*
GrallocBufferActor::Create(const gfxIntSize& aSize,
                           const gfxContentType& aContent,
                           MaybeMagicGrallocBufferHandle* aOutHandle)
{
  GrallocBufferActor* actor = new GrallocBufferActor();
  *aOutHandle = null_t();
  android::PixelFormat format = PixelFormatForContentType(aContent);
  sp<GraphicBuffer> buffer(
    new GraphicBuffer(aSize.width, aSize.height, format,
                      GraphicBuffer::USAGE_SW_READ_OFTEN |
                      GraphicBuffer::USAGE_SW_WRITE_OFTEN |
                      GraphicBuffer::USAGE_HW_TEXTURE));
  if (buffer->initCheck() != OK)
    return actor;

  actor->mGraphicBuffer = buffer;
  *aOutHandle = MagicGrallocBufferHandle(buffer);
  return actor;
}

bool
ShadowLayerManager::PlatformDestroySharedSurface(SurfaceDescriptor* aSurface)
{
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aSurface->type()) {
    return false;
  }

  PGrallocBufferParent* gbp =
    aSurface->get_SurfaceDescriptorGralloc().bufferParent();
  unused << PGrallocBufferParent::Send__delete__(gbp);
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

bool
ShadowLayerForwarder::PlatformAllocBuffer(const gfxIntSize& aSize,
                                          gfxASurface::gfxContentType aContent,
                                          uint32_t aCaps,
                                          SurfaceDescriptor* aBuffer)
{
  
  
  MaybeMagicGrallocBufferHandle handle;
  PGrallocBufferChild* gc =
    mShadowManager->SendPGrallocBufferConstructor(aSize, aContent, &handle);
  if (handle.Tnull_t == handle.type()) {
    PGrallocBufferChild::Send__delete__(gc);
    return false;
  }

  GrallocBufferActor* gba = static_cast<GrallocBufferActor*>(gc);
  gba->InitFromHandle(handle.get_MagicGrallocBufferHandle());

  *aBuffer = SurfaceDescriptorGralloc(nsnull, gc);
  return true;
}




 sp<GraphicBuffer>
GrallocBufferActor::GetFrom(const SurfaceDescriptorGralloc& aDescriptor)
{
  GrallocBufferActor* gba = nsnull;
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
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aSurface.type()) {
    return nsnull;
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

  gfxIntSize size = gfxIntSize(buffer->getWidth(), buffer->getHeight());
  gfxImageFormat format = ImageFormatForPixelFormat(buffer->getPixelFormat());
  long pixelStride = buffer->getStride();
  long byteStride = pixelStride * gfxASurface::BytePerPixelFromFormat(format);

  nsRefPtr<gfxASurface> surf =
    new gfxImageSurface((unsigned char*)vaddr, size, byteStride, format);
  return surf->CairoStatus() ? nsnull : surf.forget();
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
  *aSize = gfxIntSize(buffer->getWidth(), buffer->getHeight());
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
  if (SurfaceDescriptor::TSurfaceDescriptorGralloc != aDescriptor.type()) {
    return false;
  }

  sp<GraphicBuffer> buffer =
    GrallocBufferActor::GetFrom(aDescriptor);
  
  
  
  buffer->unlock();
  return true;
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
  
}

 void
ShadowLayerManager::PlatformSyncBeforeReplyUpdate()
{
  
}

} 
} 
