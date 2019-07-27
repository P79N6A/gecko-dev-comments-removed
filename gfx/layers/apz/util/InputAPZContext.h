




#ifndef mozilla_layers_InputAPZContext_h
#define mozilla_layers_InputAPZContext_h

#include "FrameMetrics.h"
#include "mozilla/EventForwards.h"

namespace mozilla {
namespace layers {





class MOZ_STACK_CLASS InputAPZContext
{
private:
  static ScrollableLayerGuid sGuid;
  static uint64_t sBlockId;
  static nsEventStatus sApzResponse;
  static bool sRoutedToChildProcess;

public:
  static ScrollableLayerGuid GetTargetLayerGuid();
  static uint64_t GetInputBlockId();
  static nsEventStatus GetApzResponse();
  static void SetRoutedToChildProcess();

  InputAPZContext(const ScrollableLayerGuid& aGuid,
                  const uint64_t& aBlockId,
                  const nsEventStatus& aApzResponse);
  ~InputAPZContext();

  bool WasRoutedToChildProcess();

private:
  ScrollableLayerGuid mOldGuid;
  uint64_t mOldBlockId;
  nsEventStatus mOldApzResponse;
  bool mOldRoutedToChildProcess;
};

} 
} 

#endif 
