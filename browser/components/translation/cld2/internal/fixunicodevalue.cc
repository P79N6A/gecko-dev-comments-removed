

















#include "fixunicodevalue.h"
#include "integral_types.h"

namespace CLD2 {






char32 FixUnicodeValue(char32 uv) {
  uint32 uuv = static_cast<uint32>(uv);
  if (uuv < 0x0100) {
    return kMapFullMicrosoft1252OrSpace[uuv];
  }
  if (uuv < 0xD800) {
    return uv;
  }
  if ((uuv & ~0x0F) == 0xFDD0) {              
    return 0xFFFD;
  }
  if ((uuv & ~0x0F) == 0xFDE0) {              
    return 0xFFFD;
  }
  if ((uuv & 0x00FFFE) == 0xFFFE) {           
    return 0xFFFD;
  }
  if ((0xE000 <= uuv) && (uuv <= 0x10FFFF))  {
    return uv;
  }
  
  return 0xFFFD;
}

}       

