



#include "prtypes.h"

namespace mozilla {
namespace unicode {





struct MultiCharMapping {
  PRUnichar mOriginalChar;
  PRUnichar mMappedChars[3];
};



const MultiCharMapping* SpecialUpper(PRUint32 aCh);
const MultiCharMapping* SpecialLower(PRUint32 aCh);
const MultiCharMapping* SpecialTitle(PRUint32 aCh);

} 
} 
