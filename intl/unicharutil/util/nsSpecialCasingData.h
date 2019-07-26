



#include <stdint.h>

namespace mozilla {
namespace unicode {





struct MultiCharMapping {
  char16_t mOriginalChar;
  char16_t mMappedChars[3];
};



const MultiCharMapping* SpecialUpper(uint32_t aCh);
const MultiCharMapping* SpecialLower(uint32_t aCh);
const MultiCharMapping* SpecialTitle(uint32_t aCh);

} 
} 
