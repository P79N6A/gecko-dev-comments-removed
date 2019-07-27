






#include "Key.h"

#include <algorithm>
#include "js/Value.h"
#include "jsfriendapi.h"
#include "mozilla/Endian.h"
#include "mozilla/FloatingPoint.h"
#include "mozIStorageStatement.h"
#include "nsAlgorithm.h"
#include "nsJSUtils.h"
#include "ReportInternalError.h"
#include "xpcpublic.h"

namespace mozilla {
namespace dom {
namespace indexedDB {

















































































const int MaxArrayCollapse = 3;

const int MaxRecursionDepth = 256;

nsresult
Key::EncodeJSValInternal(JSContext* aCx, JS::Handle<JS::Value> aVal,
                         uint8_t aTypeOffset, uint16_t aRecursionDepth)
{
  NS_ENSURE_TRUE(aRecursionDepth < MaxRecursionDepth, NS_ERROR_DOM_INDEXEDDB_DATA_ERR);

  static_assert(eMaxType * MaxArrayCollapse < 256,
                "Unable to encode jsvals.");

  if (aVal.isString()) {
    nsAutoJSString str;
    if (!str.init(aCx, aVal)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
    EncodeString(str, aTypeOffset);
    return NS_OK;
  }

  if (aVal.isNumber()) {
    double d = aVal.toNumber();
    if (mozilla::IsNaN(d)) {
      return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
    }
    EncodeNumber(d, eFloat + aTypeOffset);
    return NS_OK;
  }

  if (aVal.isObject()) {
    JS::Rooted<JSObject*> obj(aCx, &aVal.toObject());
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
        IDB_REPORT_INTERNAL_ERR();
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      for (uint32_t index = 0; index < length; index++) {
        JS::Rooted<JS::Value> val(aCx);
        if (!JS_GetElement(aCx, obj, index, &val)) {
          IDB_REPORT_INTERNAL_ERR();
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
      if (!js_DateIsValid(obj))  {
        return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
      }
      EncodeNumber(js_DateGetMsecSinceEpoch(obj), eDate + aTypeOffset);
      return NS_OK;
    }
  }

  return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
}


nsresult
Key::DecodeJSValInternal(const unsigned char*& aPos, const unsigned char* aEnd,
                         JSContext* aCx, uint8_t aTypeOffset, JS::MutableHandle<JS::Value> aVal,
                         uint16_t aRecursionDepth)
{
  NS_ENSURE_TRUE(aRecursionDepth < MaxRecursionDepth, NS_ERROR_DOM_INDEXEDDB_DATA_ERR);

  if (*aPos - aTypeOffset >= eArray) {
    JS::Rooted<JSObject*> array(aCx, JS_NewArrayObject(aCx, 0));
    if (!array) {
      NS_WARNING("Failed to make array!");
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    aTypeOffset += eMaxType;

    if (aTypeOffset == eMaxType * MaxArrayCollapse) {
      ++aPos;
      aTypeOffset = 0;
    }

    uint32_t index = 0;
    JS::Rooted<JS::Value> val(aCx);
    while (aPos < aEnd && *aPos - aTypeOffset != eTerminator) {
      nsresult rv = DecodeJSValInternal(aPos, aEnd, aCx, aTypeOffset,
                                        &val, aRecursionDepth + 1);
      NS_ENSURE_SUCCESS(rv, rv);

      aTypeOffset = 0;

      if (!JS_SetElement(aCx, array, index++, val)) {
        NS_WARNING("Failed to set array element!");
        IDB_REPORT_INTERNAL_ERR();
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    }

    NS_ASSERTION(aPos >= aEnd || (*aPos % eMaxType) == eTerminator,
                 "Should have found end-of-array marker");
    ++aPos;

    aVal.setObject(*array);
  }
  else if (*aPos - aTypeOffset == eString) {
    nsString key;
    DecodeString(aPos, aEnd, key);
    if (!xpc::StringToJsval(aCx, key, aVal)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  }
  else if (*aPos - aTypeOffset == eDate) {
    double msec = static_cast<double>(DecodeNumber(aPos, aEnd));
    JSObject* date = JS_NewDateObjectMsec(aCx, msec);
    if (!date) {
      IDB_WARNING("Failed to make date!");
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    aVal.setObject(*date);
  }
  else if (*aPos - aTypeOffset == eFloat) {
    aVal.setDouble(DecodeNumber(aPos, aEnd));
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

nsresult
Key::EncodeJSVal(JSContext* aCx,
                 JS::Handle<JS::Value> aVal,
                 uint8_t aTypeOffset)
{
  return EncodeJSValInternal(aCx, aVal, aTypeOffset, 0);
}

void
Key::EncodeString(const nsAString& aString, uint8_t aTypeOffset)
{
  

  
  
  uint32_t size = aString.Length() + 2;
  
  const char16_t* start = aString.BeginReading();
  const char16_t* end = aString.EndReading();
  for (const char16_t* iter = start; iter < end; ++iter) {
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

  
  for (const char16_t* iter = start; iter < end; ++iter) {
    if (*iter <= ONE_BYTE_LIMIT) {
      *(buffer++) = *iter + ONE_BYTE_ADJUST;
    }
    else if (*iter <= TWO_BYTE_LIMIT) {
      char16_t c = char16_t(*iter) + TWO_BYTE_ADJUST + 0x8000;
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


nsresult
Key::DecodeJSVal(const unsigned char*& aPos,
                 const unsigned char* aEnd,
                 JSContext* aCx,
                 uint8_t aTypeOffset,
                 JS::MutableHandle<JS::Value> aVal)
{
  return DecodeJSValInternal(aPos, aEnd, aCx, aTypeOffset, aVal, 0);
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

  char16_t* out;
  if (size && !aString.GetMutableData(&out, size)) {
    return;
  }

  for (iter = buffer; iter < aEnd;) {
    if (!(*iter & 0x80)) {
      *out = *(iter++) - ONE_BYTE_ADJUST;
    }
    else if (!(*iter & 0x40)) {
      char16_t c = (char16_t(*(iter++)) << 8);
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
      *out = (char16_t)c;
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
                    (0 - pun.u) :
                    (pun.u | PR_UINT64(0x8000000000000000));

  mozilla::BigEndian::writeUint64(buffer, number);
}


double
Key::DecodeNumber(const unsigned char*& aPos, const unsigned char* aEnd)
{
  NS_ASSERTION(*aPos % eMaxType == eFloat ||
               *aPos % eMaxType == eDate, "Don't call me!");

  ++aPos;

  uint64_t number = 0;
  memcpy(&number, aPos, std::min<size_t>(sizeof(number), aEnd - aPos));
  number = mozilla::NativeEndian::swapFromBigEndian(number);

  aPos += sizeof(number);

  Float64Union pun;
  
  
  pun.u = number & PR_UINT64(0x8000000000000000) ?
          (number & ~PR_UINT64(0x8000000000000000)) :
          (0 - number);

  return pun.d;
}

nsresult
Key::BindToStatement(mozIStorageStatement* aStatement,
                     const nsACString& aParamName) const
{
  nsresult rv = aStatement->BindBlobByName(aParamName,
    reinterpret_cast<const uint8_t*>(mBuffer.get()), mBuffer.Length());

  return NS_SUCCEEDED(rv) ? NS_OK : NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
}

nsresult
Key::SetFromStatement(mozIStorageStatement* aStatement,
                      uint32_t aIndex)
{
  uint8_t* data;
  uint32_t dataLength = 0;

  nsresult rv = aStatement->GetBlob(aIndex, &dataLength, &data);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  mBuffer.Adopt(
    reinterpret_cast<char*>(const_cast<uint8_t*>(data)), dataLength);

  return NS_OK;
}

nsresult
Key::SetFromJSVal(JSContext* aCx,
                  JS::Handle<JS::Value> aVal)
{
  mBuffer.Truncate();

  if (aVal.isNull() || aVal.isUndefined()) {
    Unset();
    return NS_OK;
  }

  nsresult rv = EncodeJSVal(aCx, aVal, 0);
  if (NS_FAILED(rv)) {
    Unset();
    return rv;
  }
  TrimBuffer();

  return NS_OK;
}

nsresult
Key::ToJSVal(JSContext* aCx,
             JS::MutableHandle<JS::Value> aVal) const
{
  if (IsUnset()) {
    aVal.setUndefined();
    return NS_OK;
  }

  const unsigned char* pos = BufferStart();
  nsresult rv = DecodeJSVal(pos, BufferEnd(), aCx, 0, aVal);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(pos >= BufferEnd());

  return NS_OK;
}

nsresult
Key::ToJSVal(JSContext* aCx,
             JS::Heap<JS::Value>& aVal) const
{
  JS::Rooted<JS::Value> value(aCx);
  nsresult rv = ToJSVal(aCx, &value);
  if (NS_SUCCEEDED(rv)) {
    aVal = value;
  }
  return rv;
}

nsresult
Key::AppendItem(JSContext* aCx, bool aFirstOfArray, JS::Handle<JS::Value> aVal)
{
  nsresult rv = EncodeJSVal(aCx, aVal, aFirstOfArray ? eMaxType : 0);
  if (NS_FAILED(rv)) {
    Unset();
    return rv;
  }

  return NS_OK;
}

#ifdef DEBUG

void
Key::Assert(bool aCondition) const
{
  MOZ_ASSERT(aCondition);
}

#endif 

} 
} 
} 
