





#include "mozilla/FloatingPoint.h"

#include "Key.h"
#include "nsIStreamBufferAccess.h"
#include "jsfriendapi.h"
#include "nsAlgorithm.h"
#include "nsJSUtils.h"
#include "xpcpublic.h"

USING_INDEXEDDB_NAMESPACE

















































































const int MaxArrayCollapse = 3;

const int MaxRecursionDepth = 256;

nsresult
Key::EncodeJSValInternal(JSContext* aCx, const jsval aVal,
                         uint8_t aTypeOffset, uint16_t aRecursionDepth)
{
  NS_ENSURE_TRUE(aRecursionDepth < MaxRecursionDepth, NS_ERROR_DOM_INDEXEDDB_DATA_ERR);

  MOZ_STATIC_ASSERT(eMaxType * MaxArrayCollapse < 256,
                    "Unable to encode jsvals.");

  if (JSVAL_IS_STRING(aVal)) {
    nsDependentJSString str;
    if (!str.init(aCx, aVal)) {
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
    EncodeString(str, aTypeOffset);
    return NS_OK;
  }

  if (JSVAL_IS_INT(aVal)) {
    EncodeNumber((double)JSVAL_TO_INT(aVal), eFloat + aTypeOffset);
    return NS_OK;
  }

  if (JSVAL_IS_DOUBLE(aVal)) {
    double d = JSVAL_TO_DOUBLE(aVal);
    if (MOZ_DOUBLE_IS_NaN(d)) {
      return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
    }
    EncodeNumber(d, eFloat + aTypeOffset);
    return NS_OK;
  }

  if (!JSVAL_IS_PRIMITIVE(aVal)) {
    JSObject* obj = JSVAL_TO_OBJECT(aVal);
    if (JS_IsArrayObject(aCx, obj)) {
      aTypeOffset += eMaxType;

      if (aTypeOffset == eMaxType * MaxArrayCollapse) {
        mBuffer.Append(aTypeOffset);
        aTypeOffset = 0;
      }
      NS_ASSERTION((aTypeOffset % eMaxType) == 0 &&
                   aTypeOffset < (eMaxType * MaxArrayCollapse),
                   "Wrong typeoffset");

      uint32_t length;
      if (!JS_GetArrayLength(aCx, obj, &length)) {
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      for (uint32_t index = 0; index < length; index++) {
        jsval val;
        if (!JS_GetElement(aCx, obj, index, &val)) {
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }

        nsresult rv = EncodeJSValInternal(aCx, val, aTypeOffset,
                                          aRecursionDepth + 1);
        if (NS_FAILED(rv)) {
          return rv;
        }

        aTypeOffset = 0;
      }

      mBuffer.Append(eTerminator + aTypeOffset);

      return NS_OK;
    }

    if (JS_ObjectIsDate(aCx, obj)) {
      if (!js_DateIsValid(aCx, obj))  {
        return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
      }
      EncodeNumber(js_DateGetMsecSinceEpoch(aCx, obj), eDate + aTypeOffset);
      return NS_OK;
    }
  }

  return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
}


nsresult
Key::DecodeJSValInternal(const unsigned char*& aPos, const unsigned char* aEnd,
                         JSContext* aCx, uint8_t aTypeOffset, jsval* aVal,
                         uint16_t aRecursionDepth)
{
  NS_ENSURE_TRUE(aRecursionDepth < MaxRecursionDepth, NS_ERROR_DOM_INDEXEDDB_DATA_ERR);

  if (*aPos - aTypeOffset >= eArray) {
    JSObject* array = JS_NewArrayObject(aCx, 0, nullptr);
    if (!array) {
      NS_WARNING("Failed to make array!");
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    aTypeOffset += eMaxType;

    if (aTypeOffset == eMaxType * MaxArrayCollapse) {
      ++aPos;
      aTypeOffset = 0;
    }

    uint32_t index = 0;
    while (aPos < aEnd && *aPos - aTypeOffset != eTerminator) {
      jsval val;
      nsresult rv = DecodeJSValInternal(aPos, aEnd, aCx, aTypeOffset,
                                        &val, aRecursionDepth + 1);
      NS_ENSURE_SUCCESS(rv, rv);

      aTypeOffset = 0;

      if (!JS_SetElement(aCx, array, index++, &val)) {
        NS_WARNING("Failed to set array element!");
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    }

    NS_ASSERTION(aPos >= aEnd || (*aPos % eMaxType) == eTerminator,
                 "Should have found end-of-array marker");
    ++aPos;

    *aVal = OBJECT_TO_JSVAL(array);
  }
  else if (*aPos - aTypeOffset == eString) {
    nsString key;
    DecodeString(aPos, aEnd, key);
    if (!xpc::StringToJsval(aCx, key, aVal)) {
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  }
  else if (*aPos - aTypeOffset == eDate) {
    double msec = static_cast<double>(DecodeNumber(aPos, aEnd));
    JSObject* date = JS_NewDateObjectMsec(aCx, msec);
    if (!date) {
      NS_WARNING("Failed to make date!");
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    *aVal = OBJECT_TO_JSVAL(date);
  }
  else if (*aPos - aTypeOffset == eFloat) {
    *aVal = DOUBLE_TO_JSVAL(DecodeNumber(aPos, aEnd));
  }
  else {
    NS_NOTREACHED("Unknown key type!");
  }

  return NS_OK;
}

#define ONE_BYTE_LIMIT 0x7E
#define TWO_BYTE_LIMIT (0x3FFF+0x7F)

#define ONE_BYTE_ADJUST 1
#define TWO_BYTE_ADJUST (-0x7F)
#define THREE_BYTE_SHIFT 6

void
Key::EncodeString(const nsAString& aString, uint8_t aTypeOffset)
{
  

  
  
  uint32_t size = aString.Length() + 2;
  
  const PRUnichar* start = aString.BeginReading();
  const PRUnichar* end = aString.EndReading();
  for (const PRUnichar* iter = start; iter < end; ++iter) {
    if (*iter > ONE_BYTE_LIMIT) {
      size += *iter > TWO_BYTE_LIMIT ? 2 : 1;
    }
  }

  
  uint32_t oldLen = mBuffer.Length();
  char* buffer;
  if (!mBuffer.GetMutableData(&buffer, oldLen + size)) {
    return;
  }
  buffer += oldLen;

  
  *(buffer++) = eString + aTypeOffset;

  
  for (const PRUnichar* iter = start; iter < end; ++iter) {
    if (*iter <= ONE_BYTE_LIMIT) {
      *(buffer++) = *iter + ONE_BYTE_ADJUST;
    }
    else if (*iter <= TWO_BYTE_LIMIT) {
      PRUnichar c = PRUnichar(*iter) + TWO_BYTE_ADJUST + 0x8000;
      *(buffer++) = (char)(c >> 8);
      *(buffer++) = (char)(c & 0xFF);
    }
    else {
      uint32_t c = (uint32_t(*iter) << THREE_BYTE_SHIFT) | 0x00C00000;
      *(buffer++) = (char)(c >> 16);
      *(buffer++) = (char)(c >> 8);
      *(buffer++) = (char)c;
    }
  }

  
  *(buffer++) = eTerminator;
  
  NS_ASSERTION(buffer == mBuffer.EndReading(), "Wrote wrong number of bytes");
}


void
Key::DecodeString(const unsigned char*& aPos, const unsigned char* aEnd,
                  nsString& aString)
{
  NS_ASSERTION(*aPos % eMaxType == eString, "Don't call me!");

  const unsigned char* buffer = aPos + 1;

  
  uint32_t size = 0;
  const unsigned char* iter; 
  for (iter = buffer; iter < aEnd && *iter != eTerminator; ++iter) {
    if (*iter & 0x80) {
      iter += (*iter & 0x40) ? 2 : 1;
    }
    ++size;
  }
  
  
  
  if (iter < aEnd) {
    aEnd = iter;
  }

  PRUnichar* out;
  if (size && !aString.GetMutableData(&out, size)) {
    return;
  }

  for (iter = buffer; iter < aEnd;) {
    if (!(*iter & 0x80)) {
      *out = *(iter++) - ONE_BYTE_ADJUST;
    }
    else if (!(*iter & 0x40)) {
      PRUnichar c = (PRUnichar(*(iter++)) << 8);
      if (iter < aEnd) {
        c |= *(iter++);
      }
      *out = c - TWO_BYTE_ADJUST - 0x8000;
    }
    else {
      uint32_t c = uint32_t(*(iter++)) << (16 - THREE_BYTE_SHIFT);
      if (iter < aEnd) {
        c |= uint32_t(*(iter++)) << (8 - THREE_BYTE_SHIFT);
      }
      if (iter < aEnd) {
        c |= *(iter++) >> THREE_BYTE_SHIFT;
      }
      *out = (PRUnichar)c;
    }
    
    ++out;
  }
  
  NS_ASSERTION(!size || out == aString.EndReading(),
               "Should have written the whole string");
  
  aPos = iter + 1;
}

union Float64Union {
  double d;
  uint64_t u;
}; 

void
Key::EncodeNumber(double aFloat, uint8_t aType)
{
  
  uint32_t oldLen = mBuffer.Length();
  char* buffer;
  if (!mBuffer.GetMutableData(&buffer, oldLen + 1 + sizeof(double))) {
    return;
  }
  buffer += oldLen;

  *(buffer++) = aType;

  Float64Union pun;
  pun.d = aFloat;
  uint64_t number = pun.u & PR_UINT64(0x8000000000000000) ?
                    -pun.u :
                    (pun.u | PR_UINT64(0x8000000000000000));

  number = NS_SWAP64(number);
  memcpy(buffer, &number, sizeof(number));
}


double
Key::DecodeNumber(const unsigned char*& aPos, const unsigned char* aEnd)
{
  NS_ASSERTION(*aPos % eMaxType == eFloat ||
               *aPos % eMaxType == eDate, "Don't call me!");

  ++aPos;

  uint64_t number = 0;
  memcpy(&number, aPos, NS_MIN<size_t>(sizeof(number), aEnd - aPos));
  number = NS_SWAP64(number);

  aPos += sizeof(number);

  Float64Union pun;
  pun.u = number & PR_UINT64(0x8000000000000000) ?
          (number & ~PR_UINT64(0x8000000000000000)) :
          -number;

  return pun.d;
}
