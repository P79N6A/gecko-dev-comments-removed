




#include "nsComplexBreaker.h"

#define TH_UNICODE
#include "rulebrk.h"

void
NS_GetComplexLineBreaks(const PRUnichar* aText, uint32_t aLength,
                        uint8_t* aBreakBefore)
{
  NS_ASSERTION(aText, "aText shouldn't be null");

  for (uint32_t i = 0; i < aLength; i++)
    aBreakBefore[i] = (0 == TrbWordBreakPos(aText, i, aText + i, aLength - i));
}

