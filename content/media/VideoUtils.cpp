




































#include "VideoUtils.h"
#include "nsMathUtils.h"
#include "prtypes.h"



bool AddOverflow32(PRUint32 a, PRUint32 b, PRUint32& aResult) {
  PRUint64 rl = static_cast<PRUint64>(a) + static_cast<PRUint64>(b);
  if (rl > PR_UINT32_MAX) {
    return false;
  }
  aResult = static_cast<PRUint32>(rl);
  return true;
}

bool MulOverflow32(PRUint32 a, PRUint32 b, PRUint32& aResult)
{
  
  
  
  PRUint64 a64 = a;
  PRUint64 b64 = b;
  PRUint64 r64 = a64 * b64;
  if (r64 > PR_UINT32_MAX)
     return false;
  aResult = static_cast<PRUint32>(r64);
  return true;
}



bool AddOverflow(PRInt64 a, PRInt64 b, PRInt64& aResult) {
  if (b < 1) {
    if (PR_INT64_MIN - b <= a) {
      aResult = a + b;
      return true;
    }
  } else if (PR_INT64_MAX - b >= a) {
    aResult = a + b;
    return true;
  }
  return false;
}




bool MulOverflow(PRInt64 a, PRInt64 b, PRInt64& aResult) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRInt64 sign = (!(a < 0) == !(b < 0)) ? 1 : -1;

  PRInt64 abs_a = (a < 0) ? -a : a;
  PRInt64 abs_b = (b < 0) ? -b : b;

  if (abs_a < 0) {
    NS_ASSERTION(a == PR_INT64_MIN, "How else can this happen?");
    if (b == 0 || b == 1) {
      aResult = a * b;
      return true;
    } else {
      return false;
    }
  }

  if (abs_b < 0) {
    NS_ASSERTION(b == PR_INT64_MIN, "How else can this happen?");
    if (a == 0 || a == 1) {
      aResult = a * b;
      return true;
    } else {
      return false;
    }
  }

  NS_ASSERTION(abs_a >= 0 && abs_b >= 0, "abs values must be non-negative");

  PRInt64 a_hi = abs_a >> 32;
  PRInt64 a_lo = abs_a & 0xFFFFFFFF;
  PRInt64 b_hi = abs_b >> 32;
  PRInt64 b_lo = abs_b & 0xFFFFFFFF;

  NS_ASSERTION((a_hi<<32) + a_lo == abs_a, "Partition must be correct");
  NS_ASSERTION((b_hi<<32) + b_lo == abs_b, "Partition must be correct");

  
  
  
  if (a_hi != 0 && b_hi != 0) {
    return false;
  }

  
  NS_ASSERTION(a_hi == 0 || b_hi == 0, "One of these must be 0");

  
  
  
  
  PRInt64 q = a_hi * b_lo + a_lo * b_hi;
  if (q > PR_INT32_MAX) {
    
    return false;
  }
  q <<= 32;

  
  PRUint64 lo = a_lo * b_lo;
  if (lo > PR_INT64_MAX) {
    return false;
  }

  
  if (!AddOverflow(q, static_cast<PRInt64>(lo), aResult)) {
    return false;
  }

  aResult *= sign;
  NS_ASSERTION(a * b == aResult, "We didn't overflow, but result is wrong!");
  return true;
}



bool FramesToUsecs(PRInt64 aFrames, PRUint32 aRate, PRInt64& aOutUsecs)
{
  PRInt64 x;
  if (!MulOverflow(aFrames, USECS_PER_S, x))
    return false;
  aOutUsecs = x / aRate;
  return true;
}



bool UsecsToFrames(PRInt64 aUsecs, PRUint32 aRate, PRInt64& aOutFrames)
{
  PRInt64 x;
  if (!MulOverflow(aUsecs, aRate, x))
    return false;
  aOutFrames = x / USECS_PER_S;
  return true;
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
