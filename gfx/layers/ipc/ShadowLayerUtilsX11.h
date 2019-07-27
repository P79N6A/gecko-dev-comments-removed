






#ifndef mozilla_layers_ShadowLayerUtilsX11_h
#define mozilla_layers_ShadowLayerUtilsX11_h

#include "ipc/IPCMessageUtils.h"
#include "mozilla/GfxMessageUtils.h"
#include "nsCOMPtr.h"                   

#define MOZ_HAVE_SURFACEDESCRIPTORX11
#define MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS

typedef unsigned long XID;
typedef XID Drawable;

class gfxXlibSurface;

namespace IPC {
class Message;
}

namespace mozilla {
namespace layers {

struct SurfaceDescriptorX11 {
  SurfaceDescriptorX11()
  { }

  explicit SurfaceDescriptorX11(gfxXlibSurface* aSurf);

  SurfaceDescriptorX11(Drawable aDrawable, XID aFormatID,
                       const gfx::IntSize& aSize);

  

  bool operator==(const SurfaceDescriptorX11& aOther) const {
    
    
    
    
    
    return mId == aOther.mId;
  }

  already_AddRefed<gfxXlibSurface> OpenForeign() const;

  Drawable mId;
  XID mFormat; 
  gfx::IntSize mSize;
};

} 
} 

namespace IPC {

template <>
struct ParamTraits<mozilla::layers::SurfaceDescriptorX11> {
  typedef mozilla::layers::SurfaceDescriptorX11 paramType;

  static void Write(Message* aMsg, const paramType& aParam) {
    WriteParam(aMsg, aParam.mId);
    WriteParam(aMsg, aParam.mSize);
    WriteParam(aMsg, aParam.mFormat);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult) {
    return (ReadParam(aMsg, aIter, &aResult->mId) &&
            ReadParam(aMsg, aIter, &aResult->mSize) &&
            ReadParam(aMsg, aIter, &aResult->mFormat));
  }
};

} 

#endif  
