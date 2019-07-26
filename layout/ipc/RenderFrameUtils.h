






#ifndef mozilla_layer_RenderFrameUtils_h
#define mozilla_layer_RenderFrameUtils_h

#include "ipc/IPCMessageUtils.h"

namespace mozilla {
namespace layout {

enum ScrollingBehavior {
  



  DEFAULT_SCROLLING,
  




  ASYNC_PAN_ZOOM,
  SCROLLING_BEHAVIOR_SENTINEL
};

} 
} 

namespace IPC {

template <>
struct ParamTraits<mozilla::layout::ScrollingBehavior>
  : public EnumSerializer<mozilla::layout::ScrollingBehavior,
                          mozilla::layout::DEFAULT_SCROLLING,
                          mozilla::layout::SCROLLING_BEHAVIOR_SENTINEL>
{};

} 

#endif 
