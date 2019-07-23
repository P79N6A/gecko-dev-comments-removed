






































#include "nsComplexBreaker.h"
#include <Carbon/Carbon.h>

void
NS_GetComplexLineBreaks(const PRUnichar* aText, PRUint32 aLength,
                        PRPackedBool* aBreakBefore)
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  TextBreakLocatorRef breakLocator;

  memset(aBreakBefore, PR_FALSE, aLength * sizeof(PRPackedBool));

  OSStatus status = UCCreateTextBreakLocator(NULL, 0, kUCTextBreakLineMask, &breakLocator);

  if (status != noErr)
    return;
     
  for (UniCharArrayOffset position = 0; position < aLength;) {
    UniCharArrayOffset offset;
    status = UCFindTextBreak(breakLocator, 
                  kUCTextBreakLineMask, 
                  position == 0 ? kUCTextBreakLeadingEdgeMask : 
                                  (kUCTextBreakLeadingEdgeMask | 
                                   kUCTextBreakIterateMask),
                  aText, 
                  aLength, 
                  position, 
                  &offset);
    if (status != noErr)
      break;        
    aBreakBefore[offset] = PR_TRUE;
    position = offset;
  }
  UCDisposeTextBreakLocator(&breakLocator);
}
