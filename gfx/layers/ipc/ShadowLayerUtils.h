







































#ifndef IPC_ShadowLayerUtils_h
#define IPC_ShadowLayerUtils_h

#include "IPC/IPCMessageUtils.h"
#include "Layers.h"

#if defined(MOZ_X11)
#  include "mozilla/layers/ShadowLayerUtilsX11.h"
#else
namespace mozilla { namespace layers {
struct SurfaceDescriptorX11 {
  bool operator==(const SurfaceDescriptorX11&) const { return false; }
};
} }
#endif

namespace IPC {

template <>
struct ParamTraits<mozilla::layers::FrameMetrics>
{
  typedef mozilla::layers::FrameMetrics paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mViewport);
    WriteParam(aMsg, aParam.mContentSize);
    WriteParam(aMsg, aParam.mViewportScrollOffset);
    WriteParam(aMsg, aParam.mDisplayPort);
    WriteParam(aMsg, aParam.mScrollId);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return (ReadParam(aMsg, aIter, &aResult->mViewport) &&
            ReadParam(aMsg, aIter, &aResult->mContentSize) &&
            ReadParam(aMsg, aIter, &aResult->mViewportScrollOffset) &&
            ReadParam(aMsg, aIter, &aResult->mDisplayPort) &&
            ReadParam(aMsg, aIter, &aResult->mScrollId));
  }
};

#if !defined(MOZ_HAVE_SURFACEDESCRIPTORX11)
template <>
struct ParamTraits<mozilla::layers::SurfaceDescriptorX11> {
  typedef mozilla::layers::SurfaceDescriptorX11 paramType;
  static void Write(Message*, const paramType&) {}
  static bool Read(const Message*, void**, paramType*) { return false; }
};
#endif  

}

#endif 
