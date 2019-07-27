






#ifndef mozilla_layers_ShadowLayerUtilsGralloc_h
#define mozilla_layers_ShadowLayerUtilsGralloc_h

#include <unistd.h>
#include <ui/GraphicBuffer.h>

#include "base/process.h"
#include "ipc/IPCMessageUtils.h"

#define MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
#define MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS

namespace mozilla {
namespace layers {

class MaybeMagicGrallocBufferHandle;
class SurfaceDescriptor;

struct GrallocBufferRef {
  base::ProcessId mOwner;
  int64_t mKey;

  GrallocBufferRef()
    : mOwner(0)
    , mKey(-1)
  {

  }

  bool operator== (const GrallocBufferRef rhs) const{
    return mOwner == rhs.mOwner && mKey == rhs.mKey;
  }
};








struct MagicGrallocBufferHandle {
  typedef android::GraphicBuffer GraphicBuffer;
  MagicGrallocBufferHandle() {}

  MagicGrallocBufferHandle(const android::sp<GraphicBuffer>& aGraphicBuffer, GrallocBufferRef ref);

  

  bool operator==(const MagicGrallocBufferHandle& aOther) const {
    return mGraphicBuffer == aOther.mGraphicBuffer;
  }

  android::sp<GraphicBuffer> mGraphicBuffer;
  GrallocBufferRef mRef;
};





android::sp<android::GraphicBuffer> GetGraphicBufferFrom(MaybeMagicGrallocBufferHandle aHandle);
android::sp<android::GraphicBuffer> GetGraphicBufferFromDesc(SurfaceDescriptor aDesc);

} 
} 

namespace IPC {

template <>
struct ParamTraits<mozilla::layers::MagicGrallocBufferHandle> {
  typedef mozilla::layers::MagicGrallocBufferHandle paramType;

  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
};

template<>
struct ParamTraits<mozilla::layers::GrallocBufferRef> {
  typedef mozilla::layers::GrallocBufferRef paramType;
  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
};


} 

#endif  
