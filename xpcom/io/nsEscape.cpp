







#include "nsEscape.h"
#include "nsMemory.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"

const int netCharType[256] =





  
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


static char*
nsEscapeCount(const char* aStr, nsEscapeMask aFlags, size_t* aOutLen)

{
  if (!aStr) {
    return 0;
  }

  size_t len = 0;
  size_t charsToEscape = 0;
  static const char hexChars[] = "0123456789ABCDEF";

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

  char* result = (char*)nsMemory::Alloc(dstSize);
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

  char16_t* resultBuffer = (char16_t*)nsMemory::Alloc(
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



const int EscapeChars[256] =

{
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  
     0,1023,   0, 512,1023,   0,1023,   0,1023,1023,1023,1023,1023,1023, 953, 784,  
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1008,1008,   0,1008,   0, 768,  
  1008,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,  
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,   0, 896,   0, 896,1023,  
     0,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,  
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896,1012, 896,1023,   0,  
     0    
};

#define NO_NEED_ESC(C) (EscapeChars[((unsigned int)(C))] & (aFlags))





























bool
NS_EscapeURL(const char* aPart, int32_t aPartLen, uint32_t aFlags,
             nsACString& aResult)
{
  if (!aPart) {
    NS_NOTREACHED("null pointer");
    return false;
  }

  static const char hexChars[] = "0123456789ABCDEF";
  if (aPartLen < 0) {
    aPartLen = strlen(aPart);
  }
  bool forced = !!(aFlags & esc_Forced);
  bool ignoreNonAscii = !!(aFlags & esc_OnlyASCII);
  bool ignoreAscii = !!(aFlags & esc_OnlyNonASCII);
  bool writing = !!(aFlags & esc_AlwaysCopy);
  bool colon = !!(aFlags & esc_Colon);

  const unsigned char* src = (const unsigned char*)aPart;

  char tempBuffer[100];
  unsigned int tempBufferPos = 0;

  bool previousIsNonASCII = false;
  for (int i = 0; i < aPartLen; ++i) {
    unsigned char c = *src++;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if ((NO_NEED_ESC(c) || (c == HEX_ESCAPE && !forced)
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
      tempBuffer[tempBufferPos++] = HEX_ESCAPE;
      tempBuffer[tempBufferPos++] = hexChars[c >> 4]; 
      tempBuffer[tempBufferPos++] = hexChars[c & 0x0f]; 
    }

    if (tempBufferPos >= sizeof(tempBuffer) - 4) {
      NS_ASSERTION(writing, "should be writing");
      tempBuffer[tempBufferPos] = '\0';
      aResult += tempBuffer;
      tempBufferPos = 0;
    }

    previousIsNonASCII = (c > 0x7f);
  }
  if (writing) {
    tempBuffer[tempBufferPos] = '\0';
    aResult += tempBuffer;
  }
  return writing;
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
