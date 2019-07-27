






#include "mozilla/DebugOnly.h"

#include "mozilla/gfx/Point.h"
#include "mozilla/layers/LayerTransactionChild.h"
#include "mozilla/layers/ShadowLayers.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/TextureHost.h"
#include "mozilla/layers/SharedBufferManagerChild.h"
#include "mozilla/layers/SharedBufferManagerParent.h"
#include "mozilla/unused.h"
#include "nsXULAppAPI.h"

#include "ShadowLayerUtilsGralloc.h"

#include "nsIMemoryReporter.h"

#include "gfxPlatform.h"
#include "gfx2DGlue.h"
#include "GLContext.h"

#include "GeckoProfiler.h"

#include "cutils/properties.h"

#include "MainThreadUtils.h"

using namespace android;
using namespace base;
using namespace mozilla::layers;
using namespace mozilla::gl;

namespace IPC {

void
ParamTraits<GrallocBufferRef>::Write(Message* aMsg,
                                     const paramType& aParam)
{
  aMsg->WriteInt(aParam.mOwner);
  aMsg->WriteInt64(aParam.mKey);
}

bool
ParamTraits<GrallocBufferRef>::Read(const Message* aMsg, void** aIter,
                                    paramType* aParam)
{
  int owner;
  int64_t index;
  if (!aMsg->ReadInt(aIter, &owner) ||
      !aMsg->ReadInt64(aIter, &index)) {
    printf_stderr("ParamTraits<GrallocBufferRef>::Read() failed to read a message\n");
    return false;
  }
  aParam->mOwner = owner;
  aParam->mKey = index;
  return true;
}

void
ParamTraits<MagicGrallocBufferHandle>::Write(Message* aMsg,
                                             const paramType& aParam)
{
#if ANDROID_VERSION >= 19
  sp<GraphicBuffer> flattenable = aParam.mGraphicBuffer;
#else
  Flattenable *flattenable = aParam.mGraphicBuffer.get();
#endif
  size_t nbytes = flattenable->getFlattenedSize();
  size_t nfds = flattenable->getFdCount();

  char data[nbytes];
  int fds[nfds];

#if ANDROID_VERSION >= 19
  
  void *pdata = (void *)data;
  int *pfds = fds;

  flattenable->flatten(pdata, nbytes, pfds, nfds);

  
  
  
  
  nbytes = flattenable->getFlattenedSize();
  nfds = flattenable->getFdCount();
#else
  flattenable->flatten(data, nbytes, fds, nfds);
#endif
  aMsg->WriteInt(aParam.mRef.mOwner);
  aMsg->WriteInt64(aParam.mRef.mKey);
  aMsg->WriteSize(nbytes);
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
  const char* data;
  int owner;
  int64_t index;

  if (!aMsg->ReadInt(aIter, &owner) ||
      !aMsg->ReadInt64(aIter, &index) ||
      !aMsg->ReadSize(aIter, &nbytes) ||
      !aMsg->ReadBytes(aIter, &data, nbytes)) {
    printf_stderr("ParamTraits<MagicGrallocBufferHandle>::Read() failed to read a message\n");
    return false;
  }

  size_t nfds = aMsg->num_fds();
  int fds[nfds];
  bool sameProcess = (XRE_GetProcessType() == GeckoProcessType_Default);
  for (size_t n = 0; n < nfds; ++n) {
    FileDescriptor fd;
    if (!aMsg->ReadFileDescriptor(aIter, &fd)) {
      printf_stderr("ParamTraits<MagicGrallocBufferHandle>::Read() failed to read file descriptors\n");
      return false;
    }
    
    
    
    
    
    
    fds[n] = fd.fd;
  }

  aResult->mRef.mOwner = owner;
  aResult->mRef.mKey = index;
  if (sameProcess) {
    aResult->mGraphicBuffer = SharedBufferManagerParent::GetGraphicBuffer(aResult->mRef);
  } else {
    if (SharedBufferManagerChild::GetSingleton()->IsValidKey(index)) {
      aResult->mGraphicBuffer = SharedBufferManagerChild::GetSingleton()->GetGraphicBuffer(index);
    }
    MOZ_ASSERT(!aResult->mGraphicBuffer.get());

    
#if ANDROID_VERSION >= 19
    sp<GraphicBuffer> buffer(new GraphicBuffer());
    const void* datap = (const void*)data;
    const int* fdsp = &fds[0];
    if (NO_ERROR != buffer->unflatten(datap, nbytes, fdsp, nfds)) {
      buffer = nullptr;
    }
#else
    sp<GraphicBuffer> buffer(new GraphicBuffer());
    Flattenable *flattenable = buffer.get();
    if (NO_ERROR != flattenable->unflatten(data, nbytes, fds, nfds)) {
      buffer = nullptr;
    }
#endif
    if(buffer.get() && !aResult->mGraphicBuffer.get()) {
      aResult->mGraphicBuffer = buffer;
    }
  }

  if (!aResult->mGraphicBuffer.get()) {
    printf_stderr("ParamTraits<MagicGrallocBufferHandle>::Read() failed to get gralloc buffer\n");
    return false;
  }

  return true;
}

} 

namespace mozilla {
namespace layers {

MagicGrallocBufferHandle::MagicGrallocBufferHandle(const sp<GraphicBuffer>& aGraphicBuffer, GrallocBufferRef ref)
  : mGraphicBuffer(aGraphicBuffer)
  , mRef(ref)
{
}




static gfxImageFormat
ImageFormatForPixelFormat(android::PixelFormat aFormat)
{
  switch (aFormat) {
  case PIXEL_FORMAT_RGBA_8888:
    return gfxImageFormat::ARGB32;
  case PIXEL_FORMAT_RGBX_8888:
    return gfxImageFormat::RGB24;
  case PIXEL_FORMAT_RGB_565:
    return gfxImageFormat::RGB16_565;
  default:
    MOZ_CRASH("Unknown gralloc pixel format");
  }
  return gfxImageFormat::ARGB32;
}

static android::PixelFormat
PixelFormatForImageFormat(gfxImageFormat aFormat)
{
  switch (aFormat) {
  case gfxImageFormat::ARGB32:
    return android::PIXEL_FORMAT_RGBA_8888;
  case gfxImageFormat::RGB24:
    return android::PIXEL_FORMAT_RGBX_8888;
  case gfxImageFormat::RGB16_565:
    return android::PIXEL_FORMAT_RGB_565;
  case gfxImageFormat::A8:
    NS_WARNING("gralloc does not support gfxImageFormat::A8");
    return android::PIXEL_FORMAT_UNKNOWN;
  default:
    MOZ_CRASH("Unknown gralloc pixel format");
  }
  return android::PIXEL_FORMAT_RGBA_8888;
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




 sp<GraphicBuffer>
GetGraphicBufferFrom(MaybeMagicGrallocBufferHandle aHandle)
{
  if (aHandle.type() != MaybeMagicGrallocBufferHandle::TMagicGrallocBufferHandle) {
    if (aHandle.type() == MaybeMagicGrallocBufferHandle::TGrallocBufferRef) {
      if (XRE_GetProcessType() == GeckoProcessType_Default) {
        return SharedBufferManagerParent::GetGraphicBuffer(aHandle.get_GrallocBufferRef());
      }
      return SharedBufferManagerChild::GetSingleton()->GetGraphicBuffer(aHandle.get_GrallocBufferRef().mKey);
    }
  } else {
    MagicGrallocBufferHandle realHandle = aHandle.get_MagicGrallocBufferHandle();
    return realHandle.mGraphicBuffer;
  }
  return nullptr;
}

android::sp<android::GraphicBuffer>
GetGraphicBufferFromDesc(SurfaceDescriptor aDesc)
{
  MaybeMagicGrallocBufferHandle handle;
  if (aDesc.type() == SurfaceDescriptor::TNewSurfaceDescriptorGralloc) {
    handle = aDesc.get_NewSurfaceDescriptorGralloc().buffer();
  }
  return GetGraphicBufferFrom(handle);
}

 void
ShadowLayerForwarder::PlatformSyncBeforeUpdate()
{
  
}

} 
} 
