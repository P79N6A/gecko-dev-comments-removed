




#include <Carbon/Carbon.h>
#include <stdint.h>
#include "nsDebug.h"
#include "nscore.h"

void
NS_GetComplexLineBreaks(const PRUnichar* aText, uint32_t aLength,
                        uint8_t* aBreakBefore)
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  TextBreakLocatorRef breakLocator;

  memset(aBreakBefore, false, aLength * sizeof(uint8_t));

  OSStatus status = UCCreateTextBreakLocator(nullptr,
                                             0,
                                             kUCTextBreakLineMask,
                                             &breakLocator);

  if (status != noErr)
    return;
     
  for (UniCharArrayOffset position = 0; position < aLength;) {
    UniCharArrayOffset offset;
    status = UCFindTextBreak(breakLocator, 
                  kUCTextBreakLineMask, 
                  position == 0 ? kUCTextBreakLeadingEdgeMask : 
                                  (kUCTextBreakLeadingEdgeMask | 
                                   kUCTextBreakIterateMask),
                  reinterpret_cast<const UniChar*>(aText),
                  aLength, 
                  position, 
                  &offset);
    if (status != noErr || offset >= aLength)
      break;        
    aBreakBefore[offset] = true;
    position = offset;
  }
  UCDisposeTextBreakLocator(&breakLocator);
}
