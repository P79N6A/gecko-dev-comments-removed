























#include "pkixder.h"

namespace mozilla { namespace pkix { namespace der {


Result
Fail(PRErrorCode errorCode)
{
  PR_SetError(errorCode, 0);
  return Failure;
}

namespace internal {


Result
ExpectTagAndGetLength(Input& input, uint8_t expectedTag, uint16_t& length)
{
  PR_ASSERT((expectedTag & 0x1F) != 0x1F); 

  uint8_t tag;
  if (input.Read(tag) != Success) {
    return Failure;
  }

  if (tag != expectedTag) {
    return Fail(SEC_ERROR_BAD_DER);
  }

  
  
  
  
  uint8_t length1;
  if (input.Read(length1) != Success) {
    return Failure;
  }
  if (!(length1 & 0x80)) {
    length = length1;
  } else if (length1 == 0x81) {
    uint8_t length2;
    if (input.Read(length2) != Success) {
      return Failure;
    }
    if (length2 < 128) {
      
      return Fail(SEC_ERROR_BAD_DER);
    }
    length = length2;
  } else if (length1 == 0x82) {
    if (input.Read(length) != Success) {
      return Failure;
    }
    if (length < 256) {
      
      return Fail(SEC_ERROR_BAD_DER);
    }
  } else {
    
    return Fail(SEC_ERROR_BAD_DER);
  }

  
  return input.EnsureLength(length);
}

} 

} } } 
