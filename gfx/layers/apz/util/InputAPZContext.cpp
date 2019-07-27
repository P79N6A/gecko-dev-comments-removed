




#include "InputAPZContext.h"

namespace mozilla {
namespace layers {

ScrollableLayerGuid InputAPZContext::sGuid;
uint64_t InputAPZContext::sBlockId = 0;
nsEventStatus InputAPZContext::sApzResponse = nsEventStatus_eIgnore;
bool InputAPZContext::sRoutedToChildProcess = false;

 ScrollableLayerGuid
InputAPZContext::GetTargetLayerGuid()
{
  return sGuid;
}

 uint64_t
InputAPZContext::GetInputBlockId()
{
  return sBlockId;
}

 nsEventStatus
InputAPZContext::GetApzResponse()
{
  return sApzResponse;
}

 void
InputAPZContext::SetRoutedToChildProcess()
{
  sRoutedToChildProcess = true;
}

InputAPZContext::InputAPZContext(const ScrollableLayerGuid& aGuid,
                                 const uint64_t& aBlockId,
                                 const nsEventStatus& aApzResponse)
  : mOldGuid(sGuid)
  , mOldBlockId(sBlockId)
  , mOldApzResponse(sApzResponse)
  , mOldRoutedToChildProcess(sRoutedToChildProcess)
{
  sGuid = aGuid;
  sBlockId = aBlockId;
  sApzResponse = aApzResponse;
  sRoutedToChildProcess = false;
}

InputAPZContext::~InputAPZContext()
{
  sGuid = mOldGuid;
  sBlockId = mOldBlockId;
  sApzResponse = mOldApzResponse;
  sRoutedToChildProcess = mOldRoutedToChildProcess;
}

bool
InputAPZContext::WasRoutedToChildProcess()
{
  return sRoutedToChildProcess;
}

} 
} 
