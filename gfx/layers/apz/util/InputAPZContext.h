




#ifndef mozilla_layers_InputAPZContext_h
#define mozilla_layers_InputAPZContext_h

#include "FrameMetrics.h"

namespace mozilla {
namespace layers {





class MOZ_STACK_CLASS InputAPZContext
{
private:
  static ScrollableLayerGuid sGuid;
  static uint64_t sBlockId;

public:
  static ScrollableLayerGuid GetTargetLayerGuid();
  static uint64_t GetInputBlockId();

  InputAPZContext(const ScrollableLayerGuid& aGuid,
                    const uint64_t& aBlockId);
  ~InputAPZContext();

private:
  ScrollableLayerGuid mOldGuid;
  uint64_t mOldBlockId;
};

}
}

#endif 
