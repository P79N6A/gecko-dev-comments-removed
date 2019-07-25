




































#include "VideoUtils.h"
#include "prtypes.h"



PRBool AddOverflow32(PRUint32 a, PRUint32 b, PRUint32& aResult) {
  PRUint64 rl = static_cast<PRUint64>(a) + static_cast<PRUint64>(b);
  if (rl > PR_UINT32_MAX) {
    return PR_FALSE;
  }
  aResult = static_cast<PRUint32>(rl);
  return true;
}

PRBool MulOverflow32(PRUint32 a, PRUint32 b, PRUint32& aResult)
{
  
  
  
  PRUint64 a64 = a;
  PRUint64 b64 = b;
  PRUint64 r64 = a64 * b64;
  if (r64 > PR_UINT32_MAX)
     return PR_FALSE;
  aResult = static_cast<PRUint32>(r64);
  return PR_TRUE;
}



PRBool AddOverflow(PRInt64 a, PRInt64 b, PRInt64& aResult) {
  if (b < 1) {
    if (PR_INT64_MIN - b <= a) {
      aResult = a + b;
      return PR_TRUE;
    }
  } else if (PR_INT64_MAX - b >= a) {
    aResult = a + b;
    return PR_TRUE;
  }
  return PR_FALSE;
}




PRBool MulOverflow(PRInt64 a, PRInt64 b, PRInt64& aResult) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRInt64 sign = (!(a < 0) == !(b < 0)) ? 1 : -1;

  PRInt64 abs_a = (a < 0) ? -a : a;
  PRInt64 abs_b = (b < 0) ? -b : b;

  if (abs_a < 0) {
    NS_ASSERTION(a == PR_INT64_MIN, "How else can this happen?");
    if (b == 0 || b == 1) {
      aResult = a * b;
      return PR_TRUE;
    } else {
      return PR_FALSE;
    }
  }

  if (abs_b < 0) {
    NS_ASSERTION(b == PR_INT64_MIN, "How else can this happen?");
    if (a == 0 || a == 1) {
      aResult = a * b;
      return PR_TRUE;
    } else {
      return PR_FALSE;
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
    return PR_FALSE;
  }

  
  NS_ASSERTION(a_hi == 0 || b_hi == 0, "One of these must be 0");

  
  
  
  
  PRInt64 q = a_hi * b_lo + a_lo * b_hi;
  if (q > PR_INT32_MAX) {
    
    return PR_FALSE;
  }
  q <<= 32;

  
  PRUint64 lo = a_lo * b_lo;
  if (lo > PR_INT64_MAX) {
    return PR_FALSE;
  }

  
  if (!AddOverflow(q, static_cast<PRInt64>(lo), aResult)) {
    return PR_FALSE;
  }

  aResult *= sign;
  NS_ASSERTION(a * b == aResult, "We didn't overflow, but result is wrong!");
  return PR_TRUE;
}
