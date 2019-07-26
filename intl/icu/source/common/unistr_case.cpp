

















#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "cmemory.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/uchar.h"
#include "uelement.h"
#include "ustr_imp.h"

U_NAMESPACE_BEGIN





int8_t
UnicodeString::doCaseCompare(int32_t start,
                             int32_t length,
                             const UChar *srcChars,
                             int32_t srcStart,
                             int32_t srcLength,
                             uint32_t options) const
{
  
  
  if(isBogus()) {
    return -1;
  }

  
  pinIndices(start, length);

  if(srcChars == NULL) {
    srcStart = srcLength = 0;
  }

  
  const UChar *chars = getArrayStart();

  chars += start;
  if(srcStart!=0) {
    srcChars += srcStart;
  }

  if(chars != srcChars) {
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t result=u_strcmpFold(chars, length, srcChars, srcLength,
                                options|U_COMPARE_IGNORE_CASE, &errorCode);
    if(result!=0) {
      return (int8_t)(result >> 24 | 1);
    }
  } else {
    
    if(srcLength < 0) {
      srcLength = u_strlen(srcChars + srcStart);
    }
    if(length != srcLength) {
      return (int8_t)((length - srcLength) >> 24 | 1);
    }
  }
  return 0;
}





UnicodeString &
UnicodeString::caseMap(const UCaseMap *csm,
                       UStringCaseMapper *stringCaseMapper) {
  if(isEmpty() || !isWritable()) {
    
    return *this;
  }

  
  
  
  
  UChar oldStackBuffer[US_STACKBUF_SIZE];
  UChar *oldArray;
  int32_t oldLength;

  if(fFlags&kUsingStackBuffer) {
    
    u_memcpy(oldStackBuffer, fUnion.fStackBuffer, fShortLength);
    oldArray = oldStackBuffer;
    oldLength = fShortLength;
  } else {
    oldArray = getArrayStart();
    oldLength = length();
  }

  int32_t capacity;
  if(oldLength <= US_STACKBUF_SIZE) {
    capacity = US_STACKBUF_SIZE;
  } else {
    capacity = oldLength + 20;
  }
  int32_t *bufferToDelete = 0;
  if(!cloneArrayIfNeeded(capacity, capacity, FALSE, &bufferToDelete, TRUE)) {
    return *this;
  }

  
  UErrorCode errorCode;
  int32_t newLength;
  do {
    errorCode = U_ZERO_ERROR;
    newLength = stringCaseMapper(csm, getArrayStart(), getCapacity(),
                                 oldArray, oldLength, &errorCode);
    setLength(newLength);
  } while(errorCode==U_BUFFER_OVERFLOW_ERROR && cloneArrayIfNeeded(newLength, newLength, FALSE));

  if (bufferToDelete) {
    uprv_free(bufferToDelete);
  }
  if(U_FAILURE(errorCode)) {
    setToBogus();
  }
  return *this;
}

UnicodeString &
UnicodeString::foldCase(uint32_t options) {
  UCaseMap csm=UCASEMAP_INITIALIZER;
  csm.csp=ucase_getSingleton();
  csm.options=options;
  return caseMap(&csm, ustrcase_internalFold);
}

U_NAMESPACE_END


U_CAPI int32_t U_EXPORT2
uhash_hashCaselessUnicodeString(const UElement key) {
    U_NAMESPACE_USE
    const UnicodeString *str = (const UnicodeString*) key.pointer;
    if (str == NULL) {
        return 0;
    }
    
    
    UnicodeString copy(*str);
    return copy.foldCase().hashCode();
}


U_CAPI UBool U_EXPORT2
uhash_compareCaselessUnicodeString(const UElement key1, const UElement key2) {
    U_NAMESPACE_USE
    const UnicodeString *str1 = (const UnicodeString*) key1.pointer;
    const UnicodeString *str2 = (const UnicodeString*) key2.pointer;
    if (str1 == str2) {
        return TRUE;
    }
    if (str1 == NULL || str2 == NULL) {
        return FALSE;
    }
    return str1->caseCompare(*str2, U_FOLD_CASE_DEFAULT) == 0;
}
