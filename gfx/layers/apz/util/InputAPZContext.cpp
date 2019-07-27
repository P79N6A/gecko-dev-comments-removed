




#include "InputAPZContext.h"

namespace mozilla {
namespace layers {

ScrollableLayerGuid InputAPZContext::sGuid;
uint64_t InputAPZContext::sBlockId = 0;
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

 void
InputAPZContext::SetRoutedToChildProcess()
{
  sRoutedToChildProcess = true;
}

InputAPZContext::InputAPZContext(const ScrollableLayerGuid& aGuid,
                                 const uint64_t& aBlockId)
  : mOldGuid(sGuid)
  , mOldBlockId(sBlockId)
  , mOldRoutedToChildProcess(sRoutedToChildProcess)
{
  sGuid = aGuid;
  sBlockId = aBlockId;
  sRoutedToChildProcess = false;
}

InputAPZContext::~InputAPZContext()
{
  sGuid = mOldGuid;
  sBlockId = mOldBlockId;
  sRoutedToChildProcess = mOldRoutedToChildProcess;
}

bool
InputAPZContext::WasRoutedToChildProcess()
{
  return sRoutedToChildProcess;
}

}
}
