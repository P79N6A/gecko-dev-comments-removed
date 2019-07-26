




 















#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsCharTraits.h"
#include "nsUTF8Utils.h"







#define IS_DELIM(m, c)          ((m)[(c) >> 3] & (1 << ((c) & 7)))
#define SET_DELIM(m, c)         ((m)[(c) >> 3] |= (1 << ((c) & 7)))
#define DELIM_TABLE_SIZE        32

char* nsCRT::strtok(char* string, const char* delims, char* *newStr)
{
  NS_ASSERTION(string, "Unlike regular strtok, the first argument cannot be null.");

  char delimTable[DELIM_TABLE_SIZE];
  uint32_t i;
  char* result;
  char* str = string;

  for (i = 0; i < DELIM_TABLE_SIZE; i++)
    delimTable[i] = '\0';

  for (i = 0; delims[i]; i++) {
    SET_DELIM(delimTable, static_cast<uint8_t>(delims[i]));
  }
  NS_ASSERTION(delims[i] == '\0', "too many delimiters");

  
  while (*str && IS_DELIM(delimTable, static_cast<uint8_t>(*str))) {
    str++;
  }
  result = str;

  
  while (*str) {
    if (IS_DELIM(delimTable, static_cast<uint8_t>(*str))) {
      *str++ = '\0';
      break;
    }
    str++;
  }
  *newStr = str;

  return str == result ? NULL : result;
}












int32_t nsCRT::strcmp(const PRUnichar* s1, const PRUnichar* s2) {
  if(s1 && s2) {
    for (;;) {
      PRUnichar c1 = *s1++;
      PRUnichar c2 = *s2++;
      if (c1 != c2) {
        if (c1 < c2) return -1;
        return 1;
      }
      if ((0==c1) || (0==c2)) break;
    }
  }
  else {
    if (s1)                     
      return -1;
    if (s2)                     
      return 1;
  }
  return 0;
}










int32_t nsCRT::strncmp(const PRUnichar* s1, const PRUnichar* s2, uint32_t n) {
  if(s1 && s2) { 
    if(n != 0) {
      do {
        PRUnichar c1 = *s1++;
        PRUnichar c2 = *s2++;
        if (c1 != c2) {
          if (c1 < c2) return -1;
          return 1;
        }
      } while (--n != 0);
    }
  }
  return 0;
}

const char* nsCRT::memmem(const char* haystack, uint32_t haystackLen,
                          const char* needle, uint32_t needleLen)
{
  
  if (!(haystack && needle && haystackLen && needleLen &&
        needleLen <= haystackLen))
    return NULL;

#ifdef HAVE_MEMMEM
  return (const char*)::memmem(haystack, haystackLen, needle, needleLen);
#else
  
  
  
  for (uint32_t i = 0; i < haystackLen - needleLen; i++) {
    if (!memcmp(haystack + i, needle, needleLen))
      return haystack + i;
  }
#endif
  return NULL;
}

PRUnichar* nsCRT::strdup(const PRUnichar* str)
{
  uint32_t len = NS_strlen(str);
  return strndup(str, len);
}

PRUnichar* nsCRT::strndup(const PRUnichar* str, uint32_t len)
{
	nsCppSharedAllocator<PRUnichar> shared_allocator;
	PRUnichar* rslt = shared_allocator.allocate(len + 1); 
  

  if (rslt == NULL) return NULL;
  memcpy(rslt, str, len * sizeof(PRUnichar));
  rslt[len] = 0;
  return rslt;
}



int64_t nsCRT::atoll(const char *str)
{
    if (!str)
        return 0;

    int64_t ll = 0;

    while (*str && *str >= '0' && *str <= '9') {
        ll *= 10;
        ll += *str - '0';
        str++;
    }

    return ll;
}

