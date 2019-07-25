






#ifndef mozilla_layers_ShadowLayerUtilsGralloc_h
#define mozilla_layers_ShadowLayerUtilsGralloc_h

#include <unistd.h>
#include <ui/GraphicBuffer.h>

#include "ipc/IPCMessageUtils.h"
#include "mozilla/layers/PGrallocBufferChild.h"
#include "mozilla/layers/PGrallocBufferParent.h"

#define MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
#define MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS

class gfxASurface;

namespace mozilla {
namespace layers {

class MaybeMagicGrallocBufferHandle;
class SurfaceDescriptorGralloc;









struct MagicGrallocBufferHandle {
  typedef android::GraphicBuffer GraphicBuffer;

  MagicGrallocBufferHandle()
  { }

  MagicGrallocBufferHandle(const android::sp<GraphicBuffer>& aGraphicBuffer);

  

  bool operator==(const MagicGrallocBufferHandle& aOther) const {
    return mGraphicBuffer == aOther.mGraphicBuffer;
  }

  android::sp<GraphicBuffer> mGraphicBuffer;
};






class GrallocBufferActor : public PGrallocBufferChild
                         , public PGrallocBufferParent
{
  friend class ShadowLayerForwarder;
  friend class ShadowLayerManager;
  friend class ImageBridgeChild;
  typedef android::GraphicBuffer GraphicBuffer;

public:
  virtual ~GrallocBufferActor() {}

  static PGrallocBufferParent*
  Create(const gfxIntSize& aSize, const gfxContentType& aContent,
         MaybeMagicGrallocBufferHandle* aOutHandle);

  static PGrallocBufferParent*
  Create(const gfxIntSize& aSize, const uint32_t& aFormat, const uint32_t& aUsage,
         MaybeMagicGrallocBufferHandle* aOutHandle);

  static PGrallocBufferChild*
  Create();

  static android::sp<GraphicBuffer>
  GetFrom(const SurfaceDescriptorGralloc& aDescriptor);

private:
  GrallocBufferActor() {}

  void InitFromHandle(const MagicGrallocBufferHandle& aHandle);

  android::sp<GraphicBuffer> mGraphicBuffer;
};

} 
} 

namespace IPC {

template <>
struct ParamTraits<mozilla::layers::MagicGrallocBufferHandle> {
  typedef mozilla::layers::MagicGrallocBufferHandle paramType;

  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
};

} 

#endif  
