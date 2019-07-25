







































#ifndef IPC_ShadowLayerUtils_h
#define IPC_ShadowLayerUtils_h

#include "IPC/IPCMessageUtils.h"
#include "Layers.h"

namespace IPC {

template <>
struct ParamTraits<mozilla::layers::FrameMetrics>
{
  typedef mozilla::layers::FrameMetrics paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mViewportSize);
    WriteParam(aMsg, aParam.mViewportScrollOffset);
    WriteParam(aMsg, aParam.mDisplayPort);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return (ReadParam(aMsg, aIter, &aResult->mViewportSize) &&
            ReadParam(aMsg, aIter, &aResult->mViewportScrollOffset) &&
            ReadParam(aMsg, aIter, &aResult->mDisplayPort));
  }
};

}

#endif 
