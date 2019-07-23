





































#include "nsComplexBreaker.h"

#define TH_UNICODE
#include "rulebrk.h"

void
NS_GetComplexLineBreaks(const PRUnichar* aText, PRUint32 aLength,
                        PRPackedBool* aBreakBefore)
{
  NS_ASSERTION(aText, "aText shouldn't be null");

  for (PRUint32 i = 0; i < aLength; i++)
    aBreakBefore[i] = (0 == TrbWordBreakPos(aText, i, aText + i, aLength - i));
}

