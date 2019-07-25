



#include "prtypes.h"
#include "mozilla/StandardInteger.h"

namespace mozilla {
namespace unicode {





struct MultiCharMapping {
  PRUnichar mOriginalChar;
  PRUnichar mMappedChars[3];
};



const MultiCharMapping* SpecialUpper(uint32_t aCh);
const MultiCharMapping* SpecialLower(uint32_t aCh);
const MultiCharMapping* SpecialTitle(uint32_t aCh);

} 
} 
