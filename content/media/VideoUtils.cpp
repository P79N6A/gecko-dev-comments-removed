




































#include "VideoUtils.h"
#include "nsMathUtils.h"
#include "prtypes.h"

#include "mozilla/StdInt.h"



CheckedInt64 FramesToUsecs(PRInt64 aFrames, PRUint32 aRate) {
  return (CheckedInt64(aFrames) * USECS_PER_S) / aRate;
}



CheckedInt64 UsecsToFrames(PRInt64 aUsecs, PRUint32 aRate) {
  return (CheckedInt64(aUsecs) * aRate) / USECS_PER_S;
}

static PRInt32 ConditionDimension(float aValue)
{
  
  if (aValue > 1.0 && aValue <= PR_INT32_MAX)
    return PRInt32(NS_round(aValue));
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
