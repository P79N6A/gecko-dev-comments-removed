







































#ifndef mozilla_layers_ShadowLayerUtilsX11_h
#define mozilla_layers_ShadowLayerUtilsX11_h

#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>

#include "IPC/IPCMessageUtils.h"

#define MOZ_HAVE_SURFACEDESCRIPTORX11
#define MOZ_HAVE_PLATFORM_SPECIFIC_LAYER_BUFFERS

class gfxXlibSurface;

namespace mozilla {
namespace layers {

struct SurfaceDescriptorX11 {
  SurfaceDescriptorX11()
  { }

  SurfaceDescriptorX11(gfxXlibSurface* aSurf);

  SurfaceDescriptorX11(const int aXid, const int aXrenderPictID, const gfxIntSize& aSize);

  

  bool operator==(const SurfaceDescriptorX11& aOther) const {
    
    
    
    
    
    return mId == aOther.mId;
  }

  already_AddRefed<gfxXlibSurface> OpenForeign() const;

  Drawable mId;
  gfxIntSize mSize;
  PictFormat mFormat;
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
