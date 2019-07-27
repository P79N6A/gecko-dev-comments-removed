





#include "nsEscape.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/BinarySearch.h"
#include "nsTArray.h"
#include "nsCRT.h"
#include "plstr.h"

static const char hexChars[] = "0123456789ABCDEF";

static const int netCharType[256] =





  
  {  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,   
     0,0,0,0,0,0,0,0,0,0,7,4,0,7,7,4,   
     7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,   
     0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,   
     
     
     7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,   
     0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,   
     7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,   
     0, };



#define UNHEX(C) \
    ((C >= '0' && C <= '9') ? C - '0' : \
     ((C >= 'A' && C <= 'F') ? C - 'A' + 10 : \
     ((C >= 'a' && C <= 'f') ? C - 'a' + 10 : 0)))


#define IS_OK(C) (netCharType[((unsigned int)(C))] & (aFlags))
#define HEX_ESCAPE '%'

static const uint32_t ENCODE_MAX_LEN = 6; 

static uint32_t
AppendPercentHex(char* aBuffer, unsigned char aChar)
{
  uint32_t i = 0;
  aBuffer[i++] = '%';
  aBuffer[i++] = hexChars[aChar >> 4]; 
  aBuffer[i++] = hexChars[aChar & 0xF]; 
  return i;
}

static uint32_t
AppendPercentHex(char16_t* aBuffer, char16_t aChar)
{
  uint32_t i = 0;
  aBuffer[i++] = '%';
  if (aChar & 0xff00) {
    aBuffer[i++] = 'u';
    aBuffer[i++] = hexChars[aChar >> 12]; 
    aBuffer[i++] = hexChars[(aChar >> 8) & 0xF]; 
  }
  aBuffer[i++] = hexChars[(aChar >> 4) & 0xF]; 
  aBuffer[i++] = hexChars[aChar & 0xF]; 
  return i;
}


static char*
nsEscapeCount(const char* aStr, nsEscapeMask aFlags, size_t* aOutLen)

{
  if (!aStr) {
    return 0;
  }

  size_t len = 0;
  size_t charsToEscape = 0;

  const unsigned char* src = (const unsigned char*)aStr;
  while (*src) {
    len++;
    if (!IS_OK(*src++)) {
      charsToEscape++;
    }
  }

  
  
  
  size_t dstSize = len + 1 + charsToEscape;
  if (dstSize <= len) {
    return 0;
  }
  dstSize += charsToEscape;
  if (dstSize < len) {
    return 0;
  }

  
  
  
  
  if (dstSize > UINT32_MAX) {
    return 0;
  }

  char* result = (char*)moz_xmalloc(dstSize);
  if (!result) {
    return 0;
  }

  unsigned char* dst = (unsigned char*)result;
  src = (const unsigned char*)aStr;
  if (aFlags == url_XPAlphas) {
    for (size_t i = 0; i < len; ++i) {
      unsigned char c = *src++;
      if (IS_OK(c)) {
        *dst++ = c;
      } else if (c == ' ') {
        *dst++ = '+';  
      } else {
        *dst++ = HEX_ESCAPE;
        *dst++ = hexChars[c >> 4];  
        *dst++ = hexChars[c & 0x0f];  
      }
    }
  } else {
    for (size_t i = 0; i < len; ++i) {
      unsigned char c = *src++;
      if (IS_OK(c)) {
        *dst++ = c;
      } else {
        *dst++ = HEX_ESCAPE;
        *dst++ = hexChars[c >> 4];  
        *dst++ = hexChars[c & 0x0f];  
      }
    }
  }

  *dst = '\0';     
  if (aOutLen) {
    *aOutLen = dst - (unsigned char*)result;
  }
  return result;
}


char*
nsEscape(const char* aStr, nsEscapeMask aFlags)

{
  if (!aStr) {
    return nullptr;
  }
  return nsEscapeCount(aStr, aFlags, nullptr);
}


char*
nsUnescape(char* aStr)

{
  nsUnescapeCount(aStr);
  return aStr;
}


int32_t
nsUnescapeCount(char* aStr)

{
  char* src = aStr;
  char* dst = aStr;
  static const char hexChars[] = "0123456789ABCDEFabcdef";

  char c1[] = " ";
  char c2[] = " ";
  char* const pc1 = c1;
  char* const pc2 = c2;

  if (!*src) {
    
    
    
    return 0;
  }

  while (*src) {
    c1[0] = *(src + 1);
    if (*(src + 1) == '\0') {
      c2[0] = '\0';
    } else {
      c2[0] = *(src + 2);
    }

    if (*src != HEX_ESCAPE || PL_strpbrk(pc1, hexChars) == 0 ||
        PL_strpbrk(pc2, hexChars) == 0) {
      *dst++ = *src++;
    } else {
      src++; 
      if (*src) {
        *dst = UNHEX(*src) << 4;
        src++;
      }
      if (*src) {
        *dst = (*dst + UNHEX(*src));
        src++;
      }
      dst++;
    }
  }

  *dst = 0;
  return (int)(dst - aStr);

} 


char*
nsEscapeHTML(const char* aString)
{
  char* rv = nullptr;
  
  uint32_t len = strlen(aString);
  if (len >= (UINT32_MAX / 6)) {
    return nullptr;
  }

  rv = (char*)NS_Alloc((6 * len) + 1);
  char* ptr = rv;

  if (rv) {
    for (; *aString != '\0'; ++aString) {
      if (*aString == '<') {
        *ptr++ = '&';
        *ptr++ = 'l';
        *ptr++ = 't';
        *ptr++ = ';';
      } else if (*aString == '>') {
        *ptr++ = '&';
        *ptr++ = 'g';
        *ptr++ = 't';
        *ptr++ = ';';
      } else if (*aString == '&') {
        *ptr++ = '&';
        *ptr++ = 'a';
        *ptr++ = 'm';
        *ptr++ = 'p';
        *ptr++ = ';';
      } else if (*aString == '"') {
        *ptr++ = '&';
        *ptr++ = 'q';
        *ptr++ = 'u';
        *ptr++ = 'o';
        *ptr++ = 't';
        *ptr++ = ';';
      } else if (*aString == '\'') {
        *ptr++ = '&';
        *ptr++ = '#';
        *ptr++ = '3';
        *ptr++ = '9';
        *ptr++ = ';';
      } else {
        *ptr++ = *aString;
      }
    }
    *ptr = '\0';
  }

  return rv;
}

char16_t*
nsEscapeHTML2(const char16_t* aSourceBuffer, int32_t aSourceBufferLen)
{
  
  if (aSourceBufferLen < 0) {
    aSourceBufferLen = NS_strlen(aSourceBuffer);
  }

  
  if (uint32_t(aSourceBufferLen) >=
      ((UINT32_MAX - sizeof(char16_t)) / (6 * sizeof(char16_t)))) {
    return nullptr;
  }

  char16_t* resultBuffer = (char16_t*)moz_xmalloc(
    aSourceBufferLen * 6 * sizeof(char16_t) + sizeof(char16_t('\0')));
  char16_t* ptr = resultBuffer;

  if (resultBuffer) {
    int32_t i;

    for (i = 0; i < aSourceBufferLen; ++i) {
      if (aSourceBuffer[i] == '<') {
        *ptr++ = '&';
        *ptr++ = 'l';
        *ptr++ = 't';
        *ptr++ = ';';
      } else if (aSourceBuffer[i] == '>') {
        *ptr++ = '&';
        *ptr++ = 'g';
        *ptr++ = 't';
        *ptr++ = ';';
      } else if (aSourceBuffer[i] == '&') {
        *ptr++ = '&';
        *ptr++ = 'a';
        *ptr++ = 'm';
        *ptr++ = 'p';
        *ptr++ = ';';
      } else if (aSourceBuffer[i] == '"') {
        *ptr++ = '&';
        *ptr++ = 'q';
        *ptr++ = 'u';
        *ptr++ = 'o';
        *ptr++ = 't';
        *ptr++ = ';';
      } else if (aSourceBuffer[i] == '\'') {
        *ptr++ = '&';
        *ptr++ = '#';
        *ptr++ = '3';
        *ptr++ = '9';
        *ptr++ = ';';
      } else {
        *ptr++ = aSourceBuffer[i];
      }
    }
    *ptr = 0;
  }

  return resultBuffer;
}


















static const uint32_t EscapeChars[256] =

{
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
     0,1023,   0, 512,1023,   0,1023, 112,1023,1023,1023,1023,1023,1023, 953, 784,  
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1008,1008,   0,1008,   0, 768,  
  1008,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,  
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896, 896, 896, 896,1023,  
     0,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,  
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896,1012, 896,1023,   0,  
     0                                                                              
};

static uint16_t dontNeedEscape(unsigned char aChar, uint32_t aFlags)
{
  return EscapeChars[(uint32_t)aChar] & aFlags;
}
static uint16_t dontNeedEscape(uint16_t aChar, uint32_t aFlags)
{
  return aChar < mozilla::ArrayLength(EscapeChars) ?
    (EscapeChars[(uint32_t)aChar]  & aFlags) : 0;
}



template<class T>
static bool
T_EscapeURL(const typename T::char_type* aPart, size_t aPartLen,
            uint32_t aFlags, T& aResult)
{
  typedef nsCharTraits<typename T::char_type> traits;
  typedef typename traits::unsigned_char_type unsigned_char_type;
  static_assert(sizeof(*aPart) == 1 || sizeof(*aPart) == 2,
                "unexpected char type");

  if (!aPart) {
    NS_NOTREACHED("null pointer");
    return false;
  }

  bool forced = !!(aFlags & esc_Forced);
  bool ignoreNonAscii = !!(aFlags & esc_OnlyASCII);
  bool ignoreAscii = !!(aFlags & esc_OnlyNonASCII);
  bool writing = !!(aFlags & esc_AlwaysCopy);
  bool colon = !!(aFlags & esc_Colon);

  auto src = reinterpret_cast<const unsigned_char_type*>(aPart);

  typename T::char_type tempBuffer[100];
  unsigned int tempBufferPos = 0;

  bool previousIsNonASCII = false;
  for (size_t i = 0; i < aPartLen; ++i) {
    unsigned_char_type c = *src++;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if ((dontNeedEscape(c, aFlags) || (c == HEX_ESCAPE && !forced)
         || (c > 0x7f && ignoreNonAscii)
         || (c > 0x20 && c < 0x7f && ignoreAscii))
        && !(c == ':' && colon)
        && !(previousIsNonASCII && c == '|' && !ignoreNonAscii)) {
      if (writing) {
        tempBuffer[tempBufferPos++] = c;
      }
    } else { 
      if (!writing) {
        aResult.Append(aPart, i);
        writing = true;
      }
      uint32_t len = ::AppendPercentHex(tempBuffer + tempBufferPos, c);
      tempBufferPos += len;
      MOZ_ASSERT(len <= ENCODE_MAX_LEN, "potential buffer overflow");
    }

    
    if (tempBufferPos >= mozilla::ArrayLength(tempBuffer) - ENCODE_MAX_LEN) {
      NS_ASSERTION(writing, "should be writing");
      aResult.Append(tempBuffer, tempBufferPos);
      tempBufferPos = 0;
    }

    previousIsNonASCII = (c > 0x7f);
  }
  if (writing) {
    aResult.Append(tempBuffer, tempBufferPos);
  }
  return writing;
}

bool
NS_EscapeURL(const char* aPart, int32_t aPartLen, uint32_t aFlags,
             nsACString& aResult)
{
  if (aPartLen < 0) {
    aPartLen = strlen(aPart);
  }
  return T_EscapeURL(aPart, aPartLen, aFlags, aResult);
}

const nsSubstring&
NS_EscapeURL(const nsSubstring& aStr, uint32_t aFlags, nsSubstring& aResult)
{
  if (T_EscapeURL<nsSubstring>(aStr.Data(), aStr.Length(), aFlags, aResult)) {
    return aResult;
  }
  return aStr;
}



static bool
FindFirstMatchFrom(const nsAFlatString& aStr, size_t aStart,
                   const nsTArray<char16_t>& aForbidden, size_t* aIndex)
{
  const size_t len = aForbidden.Length();
  for (size_t j = aStart, l = aStr.Length(); j < l; ++j) {
    size_t unused;
    if (mozilla::BinarySearch(aForbidden, 0, len, aStr[j], &unused)) {
      *aIndex = j;
      return true;
    }
  }
  return false;
}

const nsSubstring&
NS_EscapeURL(const nsAFlatString& aStr, const nsTArray<char16_t>& aForbidden,
             nsSubstring& aResult)
{
  bool didEscape = false;
  for (size_t i = 0, len = aStr.Length(); i < len; ) {
    size_t j;
    if (MOZ_UNLIKELY(FindFirstMatchFrom(aStr, i, aForbidden, &j))) {
      if (i == 0) {
        didEscape = true;
        aResult.Truncate();
        aResult.SetCapacity(aStr.Length());
      }
      if (j != i) {
        
        aResult.Append(nsDependentSubstring(aStr, i, j - i));
      }
      char16_t buffer[ENCODE_MAX_LEN];
      uint32_t len = ::AppendPercentHex(buffer, aStr[j]);
      MOZ_ASSERT(len <= ENCODE_MAX_LEN, "buffer overflow");
      aResult.Append(buffer, len);
      i = j + 1;
    } else {
      if (MOZ_UNLIKELY(didEscape)) {
        
        aResult.Append(nsDependentSubstring(aStr, i, len - i));
      }
      break;
    }
  }
  if (MOZ_UNLIKELY(didEscape)) {
    return aResult;
  }
  return aStr;
}

#define ISHEX(c) memchr(hexChars, c, sizeof(hexChars)-1)

bool
NS_UnescapeURL(const char* aStr, int32_t aLen, uint32_t aFlags,
               nsACString& aResult)
{
  if (!aStr) {
    NS_NOTREACHED("null pointer");
    return false;
  }

  if (aLen < 0) {
    aLen = strlen(aStr);
  }

  bool ignoreNonAscii = !!(aFlags & esc_OnlyASCII);
  bool ignoreAscii = !!(aFlags & esc_OnlyNonASCII);
  bool writing = !!(aFlags & esc_AlwaysCopy);
  bool skipControl = !!(aFlags & esc_SkipControl);

  static const char hexChars[] = "0123456789ABCDEFabcdef";

  const char* last = aStr;
  const char* p = aStr;

  for (int i = 0; i < aLen; ++i, ++p) {
    
    if (*p == HEX_ESCAPE && i < aLen - 2) {
      unsigned char* p1 = (unsigned char*)p + 1;
      unsigned char* p2 = (unsigned char*)p + 2;
      if (ISHEX(*p1) && ISHEX(*p2) &&
          ((*p1 < '8' && !ignoreAscii) || (*p1 >= '8' && !ignoreNonAscii)) &&
          !(skipControl &&
            (*p1 < '2' || (*p1 == '7' && (*p2 == 'f' || *p2 == 'F'))))) {
        
        writing = true;
        if (p > last) {
          
          aResult.Append(last, p - last);
          last = p;
        }
        char u = (UNHEX(*p1) << 4) + UNHEX(*p2);
        
        aResult.Append(u);
        i += 2;
        p += 2;
        last += 3;
      }
    }
  }
  if (writing && last < aStr + aLen) {
    aResult.Append(last, aStr + aLen - last);
  }

  return writing;
}
