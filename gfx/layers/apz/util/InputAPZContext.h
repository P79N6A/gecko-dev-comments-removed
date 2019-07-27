




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
  static bool sRoutedToChildProcess;

public:
  static ScrollableLayerGuid GetTargetLayerGuid();
  static uint64_t GetInputBlockId();
  static void SetRoutedToChildProcess();

  InputAPZContext(const ScrollableLayerGuid& aGuid,
                    const uint64_t& aBlockId);
  ~InputAPZContext();

  bool WasRoutedToChildProcess();

private:
  ScrollableLayerGuid mOldGuid;
  uint64_t mOldBlockId;
  bool mOldRoutedToChildProcess;
};

}
}

#endif 
