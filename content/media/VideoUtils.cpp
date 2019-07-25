



#include "VideoUtils.h"
#include "nsMathUtils.h"
#include "prtypes.h"

#include "mozilla/StandardInteger.h"



CheckedInt64 FramesToUsecs(int64_t aFrames, uint32_t aRate) {
  return (CheckedInt64(aFrames) * USECS_PER_S) / aRate;
}



CheckedInt64 UsecsToFrames(int64_t aUsecs, uint32_t aRate) {
  return (CheckedInt64(aUsecs) * aRate) / USECS_PER_S;
}

static int32_t ConditionDimension(float aValue)
{
  
  if (aValue > 1.0 && aValue <= PR_INT32_MAX)
    return int32_t(NS_round(aValue));
  return 0;
}

void ScaleDisplayByAspectRatio(nsIntSize& aDisplay, float aAspectRatio)
{
  if (aAspectRatio > 1.0) {
    
    aDisplay.width = ConditionDimension(aAspectRatio * aDisplay.width);
  } else {
    
    aDisplay.height = ConditionDimension(aDisplay.height / aAspectRatio);
  }
}
