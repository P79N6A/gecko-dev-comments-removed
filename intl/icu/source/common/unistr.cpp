



















#include "unicode/utypes.h"
#include "unicode/appendable.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "cmemory.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "uelement.h"
#include "ustr_imp.h"
#include "umutex.h"
#include "uassert.h"

#if 0

#include <iostream>
using namespace std;


void
print(const UnicodeString& s,
      const char *name)
{
  UChar c;
  cout << name << ":|";
  for(int i = 0; i < s.length(); ++i) {
    c = s[i];
    if(c>= 0x007E || c < 0x0020)
      cout << "[0x" << hex << s[i] << "]";
    else
      cout << (char) s[i];
  }
  cout << '|' << endl;
}

void
print(const UChar *s,
      int32_t len,
      const char *name)
{
  UChar c;
  cout << name << ":|";
  for(int i = 0; i < len; ++i) {
    c = s[i];
    if(c>= 0x007E || c < 0x0020)
      cout << "[0x" << hex << s[i] << "]";
    else
      cout << (char) s[i];
  }
  cout << '|' << endl;
}

#endif




static
inline void
us_arrayCopy(const UChar *src, int32_t srcStart,
         UChar *dst, int32_t dstStart, int32_t count)
{
  if(count>0) {
    uprv_memmove(dst+dstStart, src+srcStart, (size_t)(count*sizeof(*src)));
  }
}


U_CDECL_BEGIN
static UChar U_CALLCONV
UnicodeString_charAt(int32_t offset, void *context) {
    return ((icu::UnicodeString*) context)->charAt(offset);
}
U_CDECL_END

U_NAMESPACE_BEGIN




Replaceable::~Replaceable() {}
Replaceable::Replaceable() {}
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UnicodeString)

UnicodeString U_EXPORT2
operator+ (const UnicodeString &s1, const UnicodeString &s2) {
    return
        UnicodeString(s1.length()+s2.length()+1, (UChar32)0, 0).
            append(s1).
                append(s2);
}






void
UnicodeString::addRef()
{  umtx_atomic_inc((int32_t *)fUnion.fFields.fArray - 1);}

int32_t
UnicodeString::removeRef()
{ return umtx_atomic_dec((int32_t *)fUnion.fFields.fArray - 1);}

int32_t
UnicodeString::refCount() const 
{ 
    umtx_lock(NULL);
    
    
    int32_t  count = *((int32_t *)fUnion.fFields.fArray - 1);
    umtx_unlock(NULL);
    return count;
 }

void
UnicodeString::releaseArray() {
  if((fFlags & kRefCounted) && removeRef() == 0) {
    uprv_free((int32_t *)fUnion.fFields.fArray - 1);
  }
}






UnicodeString::UnicodeString()
  : fShortLength(0),
    fFlags(kShortString)
{}

UnicodeString::UnicodeString(int32_t capacity, UChar32 c, int32_t count)
  : fShortLength(0),
    fFlags(0)
{
  if(count <= 0 || (uint32_t)c > 0x10ffff) {
    
    allocate(capacity);
  } else {
    
    int32_t unitCount = U16_LENGTH(c), length = count * unitCount;
    if(capacity < length) {
      capacity = length;
    }
    if(allocate(capacity)) {
      UChar *array = getArrayStart();
      int32_t i = 0;

      
      if(unitCount == 1) {
        
        while(i < length) {
          array[i++] = (UChar)c;
        }
      } else {
        
        UChar units[U16_MAX_LENGTH];
        U16_APPEND_UNSAFE(units, i, c);

        
        i = 0;

        
        
        while(i < length) {
          int32_t unitIdx = 0;
          while(unitIdx < unitCount) {
            array[i++]=units[unitIdx++];
          }
        }
      }
    }
    setLength(length);
  }
}

UnicodeString::UnicodeString(UChar ch)
  : fShortLength(1),
    fFlags(kShortString)
{
  fUnion.fStackBuffer[0] = ch;
}

UnicodeString::UnicodeString(UChar32 ch)
  : fShortLength(0),
    fFlags(kShortString)
{
  int32_t i = 0;
  UBool isError = FALSE;
  U16_APPEND(fUnion.fStackBuffer, i, US_STACKBUF_SIZE, ch, isError);
  
  
  if(!isError) {
    fShortLength = (int8_t)i;
  }
}

UnicodeString::UnicodeString(const UChar *text)
  : fShortLength(0),
    fFlags(kShortString)
{
  doReplace(0, 0, text, 0, -1);
}

UnicodeString::UnicodeString(const UChar *text,
                             int32_t textLength)
  : fShortLength(0),
    fFlags(kShortString)
{
  doReplace(0, 0, text, 0, textLength);
}

UnicodeString::UnicodeString(UBool isTerminated,
                             const UChar *text,
                             int32_t textLength)
  : fShortLength(0),
    fFlags(kReadonlyAlias)
{
  if(text == NULL) {
    
    setToEmpty();
  } else if(textLength < -1 ||
            (textLength == -1 && !isTerminated) ||
            (textLength >= 0 && isTerminated && text[textLength] != 0)
  ) {
    setToBogus();
  } else {
    if(textLength == -1) {
      
      textLength = u_strlen(text);
    }
    setArray((UChar *)text, textLength, isTerminated ? textLength + 1 : textLength);
  }
}

UnicodeString::UnicodeString(UChar *buff,
                             int32_t buffLength,
                             int32_t buffCapacity)
  : fShortLength(0),
    fFlags(kWritableAlias)
{
  if(buff == NULL) {
    
    setToEmpty();
  } else if(buffLength < -1 || buffCapacity < 0 || buffLength > buffCapacity) {
    setToBogus();
  } else {
    if(buffLength == -1) {
      
      const UChar *p = buff, *limit = buff + buffCapacity;
      while(p != limit && *p != 0) {
        ++p;
      }
      buffLength = (int32_t)(p - buff);
    }
    setArray(buff, buffLength, buffCapacity);
  }
}

UnicodeString::UnicodeString(const char *src, int32_t length, EInvariant)
  : fShortLength(0),
    fFlags(kShortString)
{
  if(src==NULL) {
    
  } else {
    if(length<0) {
      length=(int32_t)uprv_strlen(src);
    }
    if(cloneArrayIfNeeded(length, length, FALSE)) {
      u_charsToUChars(src, getArrayStart(), length);
      setLength(length);
    } else {
      setToBogus();
    }
  }
}

#if U_CHARSET_IS_UTF8

UnicodeString::UnicodeString(const char *codepageData)
  : fShortLength(0),
    fFlags(kShortString) {
  if(codepageData != 0) {
    setToUTF8(codepageData);
  }
}

UnicodeString::UnicodeString(const char *codepageData, int32_t dataLength)
  : fShortLength(0),
    fFlags(kShortString) {
  
  if(codepageData == 0 || dataLength == 0 || dataLength < -1) {
    return;
  }
  if(dataLength == -1) {
    dataLength = (int32_t)uprv_strlen(codepageData);
  }
  setToUTF8(StringPiece(codepageData, dataLength));
}


#endif

UnicodeString::UnicodeString(const UnicodeString& that)
  : Replaceable(),
    fShortLength(0),
    fFlags(kShortString)
{
  copyFrom(that);
}

UnicodeString::UnicodeString(const UnicodeString& that,
                             int32_t srcStart)
  : Replaceable(),
    fShortLength(0),
    fFlags(kShortString)
{
  setTo(that, srcStart);
}

UnicodeString::UnicodeString(const UnicodeString& that,
                             int32_t srcStart,
                             int32_t srcLength)
  : Replaceable(),
    fShortLength(0),
    fFlags(kShortString)
{
  setTo(that, srcStart, srcLength);
}


Replaceable *
Replaceable::clone() const {
  return NULL;
}


Replaceable *
UnicodeString::clone() const {
  return new UnicodeString(*this);
}





UBool
UnicodeString::allocate(int32_t capacity) {
  if(capacity <= US_STACKBUF_SIZE) {
    fFlags = kShortString;
  } else {
    
    
    
    
    int32_t words = (int32_t)(((sizeof(int32_t) + (capacity + 1) * U_SIZEOF_UCHAR + 15) & ~15) >> 2);
    int32_t *array = (int32_t*) uprv_malloc( sizeof(int32_t) * words );
    if(array != 0) {
      
      *array++ = 1;

      
      fUnion.fFields.fArray = (UChar *)array;
      fUnion.fFields.fCapacity = (int32_t)((words - 1) * (sizeof(int32_t) / U_SIZEOF_UCHAR));
      fFlags = kLongString;
    } else {
      fShortLength = 0;
      fUnion.fFields.fArray = 0;
      fUnion.fFields.fCapacity = 0;
      fFlags = kIsBogus;
      return FALSE;
    }
  }
  return TRUE;
}




UnicodeString::~UnicodeString()
{
  releaseArray();
}





UnicodeString UnicodeString::fromUTF8(const StringPiece &utf8) {
  UnicodeString result;
  result.setToUTF8(utf8);
  return result;
}

UnicodeString UnicodeString::fromUTF32(const UChar32 *utf32, int32_t length) {
  UnicodeString result;
  int32_t capacity;
  
  
  
  if(length <= US_STACKBUF_SIZE) {
    capacity = US_STACKBUF_SIZE;
  } else {
    capacity = length + (length >> 4) + 4;
  }
  do {
    UChar *utf16 = result.getBuffer(capacity);
    int32_t length16;
    UErrorCode errorCode = U_ZERO_ERROR;
    u_strFromUTF32WithSub(utf16, result.getCapacity(), &length16,
        utf32, length,
        0xfffd,  
        NULL,    
        &errorCode);
    result.releaseBuffer(length16);
    if(errorCode == U_BUFFER_OVERFLOW_ERROR) {
      capacity = length16 + 1;  
      continue;
    } else if(U_FAILURE(errorCode)) {
      result.setToBogus();
    }
    break;
  } while(TRUE);
  return result;
}





UnicodeString &
UnicodeString::operator=(const UnicodeString &src) {
  return copyFrom(src);
}

UnicodeString &
UnicodeString::fastCopyFrom(const UnicodeString &src) {
  return copyFrom(src, TRUE);
}

UnicodeString &
UnicodeString::copyFrom(const UnicodeString &src, UBool fastCopy) {
  
  if(this == 0 || this == &src) {
    return *this;
  }

  
  if(&src == 0 || src.isBogus()) {
    setToBogus();
    return *this;
  }

  
  releaseArray();

  if(src.isEmpty()) {
    
    setToEmpty();
    return *this;
  }

  
  int32_t srcLength = src.length();
  setLength(srcLength);

  
  switch(src.fFlags) {
  case kShortString:
    
    fFlags = kShortString;
    uprv_memcpy(fUnion.fStackBuffer, src.fUnion.fStackBuffer, srcLength * U_SIZEOF_UCHAR);
    break;
  case kLongString:
    
    
    ((UnicodeString &)src).addRef();
    
    fUnion.fFields.fArray = src.fUnion.fFields.fArray;
    fUnion.fFields.fCapacity = src.fUnion.fFields.fCapacity;
    fFlags = src.fFlags;
    break;
  case kReadonlyAlias:
    if(fastCopy) {
      
      
      fUnion.fFields.fArray = src.fUnion.fFields.fArray;
      fUnion.fFields.fCapacity = src.fUnion.fFields.fCapacity;
      fFlags = src.fFlags;
      break;
    }
    
    
  case kWritableAlias:
    
    if(allocate(srcLength)) {
      uprv_memcpy(getArrayStart(), src.getArrayStart(), srcLength * U_SIZEOF_UCHAR);
      break;
    }
    
  default:
    
    
    fShortLength = 0;
    fUnion.fFields.fArray = 0;
    fUnion.fFields.fCapacity = 0;
    fFlags = kIsBogus;
    break;
  }

  return *this;
}





UnicodeString UnicodeString::unescape() const {
    UnicodeString result(length(), (UChar32)0, (int32_t)0); 
    const UChar *array = getBuffer();
    int32_t len = length();
    int32_t prev = 0;
    for (int32_t i=0;;) {
        if (i == len) {
            result.append(array, prev, len - prev);
            break;
        }
        if (array[i++] == 0x5C ) {
            result.append(array, prev, (i - 1) - prev);
            UChar32 c = unescapeAt(i); 
            if (c < 0) {
                result.remove(); 
                break; 
            }
            result.append(c);
            prev = i;
        }
    }
    return result;
}

UChar32 UnicodeString::unescapeAt(int32_t &offset) const {
    return u_unescapeAt(UnicodeString_charAt, &offset, length(), (void*)this);
}




UBool
UnicodeString::doEquals(const UnicodeString &text, int32_t len) const {
  
  
  return uprv_memcmp(getArrayStart(), text.getArrayStart(), len * U_SIZEOF_UCHAR) == 0;
}

int8_t
UnicodeString::doCompare( int32_t start,
              int32_t length,
              const UChar *srcChars,
              int32_t srcStart,
              int32_t srcLength) const
{
  
  if(isBogus()) {
    return -1;
  }
  
  
  pinIndices(start, length);

  if(srcChars == NULL) {
    
    return length == 0 ? 0 : 1;
  }

  
  const UChar *chars = getArrayStart();

  chars += start;
  srcChars += srcStart;

  int32_t minLength;
  int8_t lengthResult;

  
  if(srcLength < 0) {
    srcLength = u_strlen(srcChars + srcStart);
  }

  
  if(length != srcLength) {
    if(length < srcLength) {
      minLength = length;
      lengthResult = -1;
    } else {
      minLength = srcLength;
      lengthResult = 1;
    }
  } else {
    minLength = length;
    lengthResult = 0;
  }

  







  if(minLength > 0 && chars != srcChars) {
    int32_t result;

#   if U_IS_BIG_ENDIAN 
      
      result = uprv_memcmp(chars, srcChars, minLength * sizeof(UChar));
      if(result != 0) {
        return (int8_t)(result >> 15 | 1);
      }
#   else
      
      do {
        result = ((int32_t)*(chars++) - (int32_t)*(srcChars++));
        if(result != 0) {
          return (int8_t)(result >> 15 | 1);
        }
      } while(--minLength > 0);
#   endif
  }
  return lengthResult;
}


int8_t
UnicodeString::doCompareCodePointOrder(int32_t start,
                                       int32_t length,
                                       const UChar *srcChars,
                                       int32_t srcStart,
                                       int32_t srcLength) const
{
  
  
  if(isBogus()) {
    return -1;
  }

  
  pinIndices(start, length);

  if(srcChars == NULL) {
    srcStart = srcLength = 0;
  }

  int32_t diff = uprv_strCompare(getArrayStart() + start, length, (srcChars!=NULL)?(srcChars + srcStart):NULL, srcLength, FALSE, TRUE);
  
  if(diff!=0) {
    return (int8_t)(diff >> 15 | 1);
  } else {
    return 0;
  }
}

int32_t
UnicodeString::getLength() const {
    return length();
}

UChar
UnicodeString::getCharAt(int32_t offset) const {
  return charAt(offset);
}

UChar32
UnicodeString::getChar32At(int32_t offset) const {
  return char32At(offset);
}

UChar32
UnicodeString::char32At(int32_t offset) const
{
  int32_t len = length();
  if((uint32_t)offset < (uint32_t)len) {
    const UChar *array = getArrayStart();
    UChar32 c;
    U16_GET(array, 0, offset, len, c);
    return c;
  } else {
    return kInvalidUChar;
  }
}

int32_t
UnicodeString::getChar32Start(int32_t offset) const {
  if((uint32_t)offset < (uint32_t)length()) {
    const UChar *array = getArrayStart();
    U16_SET_CP_START(array, 0, offset);
    return offset;
  } else {
    return 0;
  }
}

int32_t
UnicodeString::getChar32Limit(int32_t offset) const {
  int32_t len = length();
  if((uint32_t)offset < (uint32_t)len) {
    const UChar *array = getArrayStart();
    U16_SET_CP_LIMIT(array, 0, offset, len);
    return offset;
  } else {
    return len;
  }
}

int32_t
UnicodeString::countChar32(int32_t start, int32_t length) const {
  pinIndices(start, length);
  
  return u_countChar32(getArrayStart()+start, length);
}

UBool
UnicodeString::hasMoreChar32Than(int32_t start, int32_t length, int32_t number) const {
  pinIndices(start, length);
  
  return u_strHasMoreChar32Than(getArrayStart()+start, length, number);
}

int32_t
UnicodeString::moveIndex32(int32_t index, int32_t delta) const {
  
  int32_t len = length();
  if(index<0) {
    index=0;
  } else if(index>len) {
    index=len;
  }

  const UChar *array = getArrayStart();
  if(delta>0) {
    U16_FWD_N(array, index, len, delta);
  } else {
    U16_BACK_N(array, 0, index, -delta);
  }

  return index;
}

void
UnicodeString::doExtract(int32_t start,
             int32_t length,
             UChar *dst,
             int32_t dstStart) const
{
  
  pinIndices(start, length);

  
  const UChar *array = getArrayStart();
  if(array + start != dst + dstStart) {
    us_arrayCopy(array, start, dst, dstStart, length);
  }
}

int32_t
UnicodeString::extract(UChar *dest, int32_t destCapacity,
                       UErrorCode &errorCode) const {
  int32_t len = length();
  if(U_SUCCESS(errorCode)) {
    if(isBogus() || destCapacity<0 || (destCapacity>0 && dest==0)) {
      errorCode=U_ILLEGAL_ARGUMENT_ERROR;
    } else {
      const UChar *array = getArrayStart();
      if(len>0 && len<=destCapacity && array!=dest) {
        uprv_memcpy(dest, array, len*U_SIZEOF_UCHAR);
      }
      return u_terminateUChars(dest, destCapacity, len, &errorCode);
    }
  }

  return len;
}

int32_t
UnicodeString::extract(int32_t start,
                       int32_t length,
                       char *target,
                       int32_t targetCapacity,
                       enum EInvariant) const
{
  
  if(targetCapacity < 0 || (targetCapacity > 0 && target == NULL)) {
    return 0;
  }

  
  pinIndices(start, length);

  if(length <= targetCapacity) {
    u_UCharsToChars(getArrayStart() + start, target, length);
  }
  UErrorCode status = U_ZERO_ERROR;
  return u_terminateChars(target, targetCapacity, length, &status);
}

UnicodeString
UnicodeString::tempSubString(int32_t start, int32_t len) const {
  pinIndices(start, len);
  const UChar *array = getBuffer();  
  if(array==NULL) {
    array=fUnion.fStackBuffer;  
    len=-2;  
  }
  return UnicodeString(FALSE, array + start, len);
}

int32_t
UnicodeString::toUTF8(int32_t start, int32_t len,
                      char *target, int32_t capacity) const {
  pinIndices(start, len);
  int32_t length8;
  UErrorCode errorCode = U_ZERO_ERROR;
  u_strToUTF8WithSub(target, capacity, &length8,
                     getBuffer() + start, len,
                     0xFFFD,  
                     NULL,    
                     &errorCode);
  return length8;
}

#if U_CHARSET_IS_UTF8

int32_t
UnicodeString::extract(int32_t start, int32_t len,
                       char *target, uint32_t dstSize) const {
  
  if((dstSize > 0 && target == 0)) {
    return 0;
  }
  return toUTF8(start, len, target, dstSize <= 0x7fffffff ? (int32_t)dstSize : 0x7fffffff);
}


#endif

void 
UnicodeString::extractBetween(int32_t start,
                  int32_t limit,
                  UnicodeString& target) const {
  pinIndex(start);
  pinIndex(limit);
  doExtract(start, limit - start, target);
}





void
UnicodeString::toUTF8(ByteSink &sink) const {
  int32_t length16 = length();
  if(length16 != 0) {
    char stackBuffer[1024];
    int32_t capacity = (int32_t)sizeof(stackBuffer);
    UBool utf8IsOwned = FALSE;
    char *utf8 = sink.GetAppendBuffer(length16 < capacity ? length16 : capacity,
                                      3*length16,
                                      stackBuffer, capacity,
                                      &capacity);
    int32_t length8 = 0;
    UErrorCode errorCode = U_ZERO_ERROR;
    u_strToUTF8WithSub(utf8, capacity, &length8,
                       getBuffer(), length16,
                       0xFFFD,  
                       NULL,    
                       &errorCode);
    if(errorCode == U_BUFFER_OVERFLOW_ERROR) {
      utf8 = (char *)uprv_malloc(length8);
      if(utf8 != NULL) {
        utf8IsOwned = TRUE;
        errorCode = U_ZERO_ERROR;
        u_strToUTF8WithSub(utf8, length8, &length8,
                           getBuffer(), length16,
                           0xFFFD,  
                           NULL,    
                           &errorCode);
      } else {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
      }
    }
    if(U_SUCCESS(errorCode)) {
      sink.Append(utf8, length8);
      sink.Flush();
    }
    if(utf8IsOwned) {
      uprv_free(utf8);
    }
  }
}

int32_t
UnicodeString::toUTF32(UChar32 *utf32, int32_t capacity, UErrorCode &errorCode) const {
  int32_t length32=0;
  if(U_SUCCESS(errorCode)) {
    
    u_strToUTF32WithSub(utf32, capacity, &length32,
        getBuffer(), length(),
        0xfffd,  
        NULL,    
        &errorCode);
  }
  return length32;
}

int32_t 
UnicodeString::indexOf(const UChar *srcChars,
               int32_t srcStart,
               int32_t srcLength,
               int32_t start,
               int32_t length) const
{
  if(isBogus() || srcChars == 0 || srcStart < 0 || srcLength == 0) {
    return -1;
  }

  
  if(srcLength < 0 && srcChars[srcStart] == 0) {
    return -1;
  }

  
  pinIndices(start, length);

  
  const UChar *array = getArrayStart();
  const UChar *match = u_strFindFirst(array + start, length, srcChars + srcStart, srcLength);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t
UnicodeString::doIndexOf(UChar c,
             int32_t start,
             int32_t length) const
{
  
  pinIndices(start, length);

  
  const UChar *array = getArrayStart();
  const UChar *match = u_memchr(array + start, c, length);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t
UnicodeString::doIndexOf(UChar32 c,
                         int32_t start,
                         int32_t length) const {
  
  pinIndices(start, length);

  
  const UChar *array = getArrayStart();
  const UChar *match = u_memchr32(array + start, c, length);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t 
UnicodeString::lastIndexOf(const UChar *srcChars,
               int32_t srcStart,
               int32_t srcLength,
               int32_t start,
               int32_t length) const
{
  if(isBogus() || srcChars == 0 || srcStart < 0 || srcLength == 0) {
    return -1;
  }

  
  if(srcLength < 0 && srcChars[srcStart] == 0) {
    return -1;
  }

  
  pinIndices(start, length);

  
  const UChar *array = getArrayStart();
  const UChar *match = u_strFindLast(array + start, length, srcChars + srcStart, srcLength);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t
UnicodeString::doLastIndexOf(UChar c,
                 int32_t start,
                 int32_t length) const
{
  if(isBogus()) {
    return -1;
  }

  
  pinIndices(start, length);

  
  const UChar *array = getArrayStart();
  const UChar *match = u_memrchr(array + start, c, length);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}

int32_t
UnicodeString::doLastIndexOf(UChar32 c,
                             int32_t start,
                             int32_t length) const {
  
  pinIndices(start, length);

  
  const UChar *array = getArrayStart();
  const UChar *match = u_memrchr32(array + start, c, length);
  if(match == NULL) {
    return -1;
  } else {
    return (int32_t)(match - array);
  }
}





UnicodeString& 
UnicodeString::findAndReplace(int32_t start,
                  int32_t length,
                  const UnicodeString& oldText,
                  int32_t oldStart,
                  int32_t oldLength,
                  const UnicodeString& newText,
                  int32_t newStart,
                  int32_t newLength)
{
  if(isBogus() || oldText.isBogus() || newText.isBogus()) {
    return *this;
  }

  pinIndices(start, length);
  oldText.pinIndices(oldStart, oldLength);
  newText.pinIndices(newStart, newLength);

  if(oldLength == 0) {
    return *this;
  }

  while(length > 0 && length >= oldLength) {
    int32_t pos = indexOf(oldText, oldStart, oldLength, start, length);
    if(pos < 0) {
      
      break;
    } else {
      
      replace(pos, oldLength, newText, newStart, newLength);
      length -= pos + oldLength - start;
      start = pos + newLength;
    }
  }

  return *this;
}


void
UnicodeString::setToBogus()
{
  releaseArray();

  fShortLength = 0;
  fUnion.fFields.fArray = 0;
  fUnion.fFields.fCapacity = 0;
  fFlags = kIsBogus;
}


void
UnicodeString::unBogus() {
  if(fFlags & kIsBogus) {
    setToEmpty();
  }
}


UnicodeString &
UnicodeString::setTo(UBool isTerminated,
                     const UChar *text,
                     int32_t textLength)
{
  if(fFlags & kOpenGetBuffer) {
    
    return *this;
  }

  if(text == NULL) {
    
    releaseArray();
    setToEmpty();
    return *this;
  }

  if( textLength < -1 ||
      (textLength == -1 && !isTerminated) ||
      (textLength >= 0 && isTerminated && text[textLength] != 0)
  ) {
    setToBogus();
    return *this;
  }

  releaseArray();

  if(textLength == -1) {
    
    textLength = u_strlen(text);
  }
  setArray((UChar *)text, textLength, isTerminated ? textLength + 1 : textLength);

  fFlags = kReadonlyAlias;
  return *this;
}


UnicodeString &
UnicodeString::setTo(UChar *buffer,
                     int32_t buffLength,
                     int32_t buffCapacity) {
  if(fFlags & kOpenGetBuffer) {
    
    return *this;
  }

  if(buffer == NULL) {
    
    releaseArray();
    setToEmpty();
    return *this;
  }

  if(buffLength < -1 || buffCapacity < 0 || buffLength > buffCapacity) {
    setToBogus();
    return *this;
  } else if(buffLength == -1) {
    
    const UChar *p = buffer, *limit = buffer + buffCapacity;
    while(p != limit && *p != 0) {
      ++p;
    }
    buffLength = (int32_t)(p - buffer);
  }

  releaseArray();

  setArray(buffer, buffLength, buffCapacity);
  fFlags = kWritableAlias;
  return *this;
}

UnicodeString &UnicodeString::setToUTF8(const StringPiece &utf8) {
  unBogus();
  int32_t length = utf8.length();
  int32_t capacity;
  
  if(length <= US_STACKBUF_SIZE) {
    capacity = US_STACKBUF_SIZE;
  } else {
    capacity = length + 1;  
  }
  UChar *utf16 = getBuffer(capacity);
  int32_t length16;
  UErrorCode errorCode = U_ZERO_ERROR;
  u_strFromUTF8WithSub(utf16, getCapacity(), &length16,
      utf8.data(), length,
      0xfffd,  
      NULL,    
      &errorCode);
  releaseBuffer(length16);
  if(U_FAILURE(errorCode)) {
    setToBogus();
  }
  return *this;
}

UnicodeString&
UnicodeString::setCharAt(int32_t offset,
             UChar c)
{
  int32_t len = length();
  if(cloneArrayIfNeeded() && len > 0) {
    if(offset < 0) {
      offset = 0;
    } else if(offset >= len) {
      offset = len - 1;
    }

    getArrayStart()[offset] = c;
  }
  return *this;
}

UnicodeString&
UnicodeString::replace(int32_t start,
               int32_t _length,
               UChar32 srcChar) {
  UChar buffer[U16_MAX_LENGTH];
  int32_t count = 0;
  UBool isError = FALSE;
  U16_APPEND(buffer, count, U16_MAX_LENGTH, srcChar, isError);
  
  
  
  return doReplace(start, _length, buffer, 0, isError ? 0 : count);
}

UnicodeString&
UnicodeString::append(UChar32 srcChar) {
  UChar buffer[U16_MAX_LENGTH];
  int32_t _length = 0;
  UBool isError = FALSE;
  U16_APPEND(buffer, _length, U16_MAX_LENGTH, srcChar, isError);
  
  
  return isError ? *this : doReplace(length(), 0, buffer, 0, _length);
}

UnicodeString&
UnicodeString::doReplace( int32_t start,
              int32_t length,
              const UnicodeString& src,
              int32_t srcStart,
              int32_t srcLength)
{
  if(!src.isBogus()) {
    
    src.pinIndices(srcStart, srcLength);

    
    
    return doReplace(start, length, src.getArrayStart(), srcStart, srcLength);
  } else {
    
    return doReplace(start, length, 0, 0, 0);
  }
}

UnicodeString&
UnicodeString::doReplace(int32_t start,
             int32_t length,
             const UChar *srcChars,
             int32_t srcStart,
             int32_t srcLength)
{
  if(!isWritable()) {
    return *this;
  }

  int32_t oldLength = this->length();

  
  if((fFlags&kBufferIsReadonly) && srcLength == 0) {
    if(start == 0) {
      
      pinIndex(length);
      fUnion.fFields.fArray += length;
      fUnion.fFields.fCapacity -= length;
      setLength(oldLength - length);
      return *this;
    } else {
      pinIndex(start);
      if(length >= (oldLength - start)) {
        
        setLength(start);
        fUnion.fFields.fCapacity = start;  
        return *this;
      }
    }
  }

  if(srcChars == 0) {
    srcStart = srcLength = 0;
  } else if(srcLength < 0) {
    
    srcLength = u_strlen(srcChars + srcStart);
  }

  
  int32_t newLength;

  
  if(start >= oldLength) {
    if(srcLength == 0) {
      return *this;
    }
    newLength = oldLength + srcLength;
    if(newLength <= getCapacity() && isBufferWritable()) {
      UChar *oldArray = getArrayStart();
      
      
      
      
      
      
      
      if(srcChars + srcStart != oldArray + start || start > oldLength) {
        us_arrayCopy(srcChars, srcStart, oldArray, oldLength, srcLength);
      }
      setLength(newLength);
      return *this;
    } else {
      
      start = oldLength;
      length = 0;
    }
  } else {
    
    pinIndices(start, length);

    newLength = oldLength - length + srcLength;
  }

  
  
  UChar oldStackBuffer[US_STACKBUF_SIZE];
  UChar *oldArray;
  if((fFlags&kUsingStackBuffer) && (newLength > US_STACKBUF_SIZE)) {
    
    
    u_memcpy(oldStackBuffer, fUnion.fStackBuffer, oldLength);
    oldArray = oldStackBuffer;
  } else {
    oldArray = getArrayStart();
  }

  
  int32_t *bufferToDelete = 0;
  if(!cloneArrayIfNeeded(newLength, newLength + (newLength >> 2) + kGrowSize,
                         FALSE, &bufferToDelete)
  ) {
    return *this;
  }

  

  UChar *newArray = getArrayStart();
  if(newArray != oldArray) {
    
    us_arrayCopy(oldArray, 0, newArray, 0, start);
    us_arrayCopy(oldArray, start + length,
                 newArray, start + srcLength,
                 oldLength - (start + length));
  } else if(length != srcLength) {
    
    us_arrayCopy(oldArray, start + length,
                 newArray, start + srcLength,
                 oldLength - (start + length));
  }

  
  us_arrayCopy(srcChars, srcStart, newArray, start, srcLength);

  setLength(newLength);

  
  
  if (bufferToDelete) {
    uprv_free(bufferToDelete);
  }

  return *this;
}




void
UnicodeString::handleReplaceBetween(int32_t start,
                                    int32_t limit,
                                    const UnicodeString& text) {
    replaceBetween(start, limit, text);
}




void 
UnicodeString::copy(int32_t start, int32_t limit, int32_t dest) {
    if (limit <= start) {
        return; 
    }
    UChar* text = (UChar*) uprv_malloc( sizeof(UChar) * (limit - start) );
    
    if (text != NULL) {
	    extractBetween(start, limit, text, 0);
	    insert(dest, text, 0, limit - start);    
	    uprv_free(text);
    }
}







UBool Replaceable::hasMetaData() const {
    return TRUE;
}




UBool UnicodeString::hasMetaData() const {
    return FALSE;
}

UnicodeString&
UnicodeString::doReverse(int32_t start, int32_t length) {
  if(length <= 1 || !cloneArrayIfNeeded()) {
    return *this;
  }

  
  pinIndices(start, length);
  if(length <= 1) {  
    return *this;
  }

  UChar *left = getArrayStart() + start;
  UChar *right = left + length - 1;  
  UChar swap;
  UBool hasSupplementary = FALSE;

  
  do {
    hasSupplementary |= (UBool)U16_IS_LEAD(swap = *left);
    hasSupplementary |= (UBool)U16_IS_LEAD(*left++ = *right);
    *right-- = swap;
  } while(left < right);
  
  
  hasSupplementary |= (UBool)U16_IS_LEAD(*left);

  
  if(hasSupplementary) {
    UChar swap2;

    left = getArrayStart() + start;
    right = left + length - 1; 
    while(left < right) {
      if(U16_IS_TRAIL(swap = *left) && U16_IS_LEAD(swap2 = *(left + 1))) {
        *left++ = swap2;
        *left++ = swap;
      } else {
        ++left;
      }
    }
  }

  return *this;
}

UBool 
UnicodeString::padLeading(int32_t targetLength,
                          UChar padChar)
{
  int32_t oldLength = length();
  if(oldLength >= targetLength || !cloneArrayIfNeeded(targetLength)) {
    return FALSE;
  } else {
    
    UChar *array = getArrayStart();
    int32_t start = targetLength - oldLength;
    us_arrayCopy(array, 0, array, start, oldLength);

    
    while(--start >= 0) {
      array[start] = padChar;
    }
    setLength(targetLength);
    return TRUE;
  }
}

UBool 
UnicodeString::padTrailing(int32_t targetLength,
                           UChar padChar)
{
  int32_t oldLength = length();
  if(oldLength >= targetLength || !cloneArrayIfNeeded(targetLength)) {
    return FALSE;
  } else {
    
    UChar *array = getArrayStart();
    int32_t length = targetLength;
    while(--length >= oldLength) {
      array[length] = padChar;
    }
    setLength(targetLength);
    return TRUE;
  }
}




int32_t
UnicodeString::doHashCode() const
{
    

    int32_t hashCode = ustr_hashUCharsN(getArrayStart(), length());
    if (hashCode == kInvalidHashCode) {
        hashCode = kEmptyHashCode;
    }
    return hashCode;
}





UChar *
UnicodeString::getBuffer(int32_t minCapacity) {
  if(minCapacity>=-1 && cloneArrayIfNeeded(minCapacity)) {
    fFlags|=kOpenGetBuffer;
    fShortLength=0;
    return getArrayStart();
  } else {
    return 0;
  }
}

void
UnicodeString::releaseBuffer(int32_t newLength) {
  if(fFlags&kOpenGetBuffer && newLength>=-1) {
    
    int32_t capacity=getCapacity();
    if(newLength==-1) {
      
      const UChar *array=getArrayStart(), *p=array, *limit=array+capacity;
      while(p<limit && *p!=0) {
        ++p;
      }
      newLength=(int32_t)(p-array);
    } else if(newLength>capacity) {
      newLength=capacity;
    }
    setLength(newLength);
    fFlags&=~kOpenGetBuffer;
  }
}




UBool
UnicodeString::cloneArrayIfNeeded(int32_t newCapacity,
                                  int32_t growCapacity,
                                  UBool doCopyArray,
                                  int32_t **pBufferToDelete,
                                  UBool forceClone) {
  
  
  if(newCapacity == -1) {
    newCapacity = getCapacity();
  }

  
  
  
  if(!isWritable()) {
    return FALSE;
  }

  






  if(forceClone ||
     fFlags & kBufferIsReadonly ||
     (fFlags & kRefCounted && refCount() > 1) ||
     newCapacity > getCapacity()
  ) {
    
    if(growCapacity < 0) {
      growCapacity = newCapacity;
    } else if(newCapacity <= US_STACKBUF_SIZE && growCapacity > US_STACKBUF_SIZE) {
      growCapacity = US_STACKBUF_SIZE;
    }

    
    UChar oldStackBuffer[US_STACKBUF_SIZE];
    UChar *oldArray;
    uint8_t flags = fFlags;

    if(flags&kUsingStackBuffer) {
      U_ASSERT(!(flags&kRefCounted)); 
      if(doCopyArray && growCapacity > US_STACKBUF_SIZE) {
        
        
        us_arrayCopy(fUnion.fStackBuffer, 0, oldStackBuffer, 0, fShortLength);
        oldArray = oldStackBuffer;
      } else {
        oldArray = 0; 
      }
    } else {
      oldArray = fUnion.fFields.fArray;
      U_ASSERT(oldArray!=NULL); 
    }

    
    if(allocate(growCapacity) ||
       (newCapacity < growCapacity && allocate(newCapacity))
    ) {
      if(doCopyArray && oldArray != 0) {
        
        
        int32_t minLength = length();
        newCapacity = getCapacity();
        if(newCapacity < minLength) {
          minLength = newCapacity;
          setLength(minLength);
        }
        us_arrayCopy(oldArray, 0, getArrayStart(), 0, minLength);
      } else {
        fShortLength = 0;
      }

      
      if(flags & kRefCounted) {
        
        int32_t *pRefCount = ((int32_t *)oldArray - 1);
        if(umtx_atomic_dec(pRefCount) == 0) {
          if(pBufferToDelete == 0) {
            uprv_free(pRefCount);
          } else {
            
            *pBufferToDelete = pRefCount;
          }
        }
      }
    } else {
      
      
      if(!(flags&kUsingStackBuffer)) {
        fUnion.fFields.fArray = oldArray;
      }
      fFlags = flags;
      setToBogus();
      return FALSE;
    }
  }
  return TRUE;
}



UnicodeStringAppendable::~UnicodeStringAppendable() {}

UBool
UnicodeStringAppendable::appendCodeUnit(UChar c) {
  return str.doReplace(str.length(), 0, &c, 0, 1).isWritable();
}

UBool
UnicodeStringAppendable::appendCodePoint(UChar32 c) {
  UChar buffer[U16_MAX_LENGTH];
  int32_t cLength = 0;
  UBool isError = FALSE;
  U16_APPEND(buffer, cLength, U16_MAX_LENGTH, c, isError);
  return !isError && str.doReplace(str.length(), 0, buffer, 0, cLength).isWritable();
}

UBool
UnicodeStringAppendable::appendString(const UChar *s, int32_t length) {
  return str.doReplace(str.length(), 0, s, 0, length).isWritable();
}

UBool
UnicodeStringAppendable::reserveAppendCapacity(int32_t appendCapacity) {
  return str.cloneArrayIfNeeded(str.length() + appendCapacity);
}

UChar *
UnicodeStringAppendable::getAppendBuffer(int32_t minCapacity,
                                         int32_t desiredCapacityHint,
                                         UChar *scratch, int32_t scratchCapacity,
                                         int32_t *resultCapacity) {
  if(minCapacity < 1 || scratchCapacity < minCapacity) {
    *resultCapacity = 0;
    return NULL;
  }
  int32_t oldLength = str.length();
  if(str.cloneArrayIfNeeded(oldLength + minCapacity, oldLength + desiredCapacityHint)) {
    *resultCapacity = str.getCapacity() - oldLength;
    return str.getArrayStart() + oldLength;
  }
  *resultCapacity = scratchCapacity;
  return scratch;
}

U_NAMESPACE_END

U_NAMESPACE_USE

U_CAPI int32_t U_EXPORT2
uhash_hashUnicodeString(const UElement key) {
    const UnicodeString *str = (const UnicodeString*) key.pointer;
    return (str == NULL) ? 0 : str->hashCode();
}



U_CAPI UBool U_EXPORT2
uhash_compareUnicodeString(const UElement key1, const UElement key2) {
    const UnicodeString *str1 = (const UnicodeString*) key1.pointer;
    const UnicodeString *str2 = (const UnicodeString*) key2.pointer;
    if (str1 == str2) {
        return TRUE;
    }
    if (str1 == NULL || str2 == NULL) {
        return FALSE;
    }
    return *str1 == *str2;
}

#ifdef U_STATIC_IMPLEMENTATION







static void uprv_UnicodeStringDummy(void) {
    delete [] (new UnicodeString[2]);
}
#endif
