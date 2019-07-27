




#include "InputAPZContext.h"

namespace mozilla {
namespace layers {

ScrollableLayerGuid InputAPZContext::sGuid;
uint64_t InputAPZContext::sBlockId = 0;

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

InputAPZContext::InputAPZContext(const ScrollableLayerGuid& aGuid,
                                 const uint64_t& aBlockId)
  : mOldGuid(sGuid)
  , mOldBlockId(sBlockId)
{
  sGuid = aGuid;
  sBlockId = aBlockId;
}

InputAPZContext::~InputAPZContext()
{
  sGuid = mOldGuid;
  sBlockId = mOldBlockId;
}

}
}
