





















#include "nsCRT.h"
#include "nsDebug.h"







#define IS_DELIM(m, c)          ((m)[(c) >> 3] & (1 << ((c) & 7)))
#define SET_DELIM(m, c)         ((m)[(c) >> 3] |= (1 << ((c) & 7)))
#define DELIM_TABLE_SIZE        32

char*
nsCRT::strtok(char* aString, const char* aDelims, char** aNewStr)
{
  NS_ASSERTION(aString,
               "Unlike regular strtok, the first argument cannot be null.");

  char delimTable[DELIM_TABLE_SIZE];
  uint32_t i;
  char* result;
  char* str = aString;

  for (i = 0; i < DELIM_TABLE_SIZE; ++i) {
    delimTable[i] = '\0';
  }

  for (i = 0; aDelims[i]; i++) {
    SET_DELIM(delimTable, static_cast<uint8_t>(aDelims[i]));
  }
  NS_ASSERTION(aDelims[i] == '\0', "too many delimiters");

  
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
  *aNewStr = str;

  return str == result ? nullptr : result;
}












int32_t
nsCRT::strcmp(const char16_t* aStr1, const char16_t* aStr2)
{
  if (aStr1 && aStr2) {
    for (;;) {
      char16_t c1 = *aStr1++;
      char16_t c2 = *aStr2++;
      if (c1 != c2) {
        if (c1 < c2) {
          return -1;
        }
        return 1;
      }
      if (c1 == 0 || c2 == 0) {
        break;
      }
    }
  } else {
    if (aStr1) {  
      return -1;
    }
    if (aStr2) {  
      return 1;
    }
  }
  return 0;
}










int32_t
nsCRT::strncmp(const char16_t* aStr1, const char16_t* aStr2, uint32_t aNum)
{
  if (aStr1 && aStr2) {
    if (aNum != 0) {
      do {
        char16_t c1 = *aStr1++;
        char16_t c2 = *aStr2++;
        if (c1 != c2) {
          if (c1 < c2) {
            return -1;
          }
          return 1;
        }
      } while (--aNum != 0);
    }
  }
  return 0;
}

const char*
nsCRT::memmem(const char* aHaystack, uint32_t aHaystackLen,
              const char* aNeedle, uint32_t aNeedleLen)
{
  
  if (!(aHaystack && aNeedle && aHaystackLen && aNeedleLen &&
        aNeedleLen <= aHaystackLen)) {
    return nullptr;
  }

#ifdef HAVE_MEMMEM
  return (const char*)::memmem(aHaystack, aHaystackLen, aNeedle, aNeedleLen);
#else
  
  
  
  for (uint32_t i = 0; i < aHaystackLen - aNeedleLen; i++) {
    if (!memcmp(aHaystack + i, aNeedle, aNeedleLen)) {
      return aHaystack + i;
    }
  }
#endif
  return nullptr;
}



int64_t
nsCRT::atoll(const char* aStr)
{
  if (!aStr) {
    return 0;
  }

  int64_t ll = 0;

  while (*aStr && *aStr >= '0' && *aStr <= '9') {
    ll *= 10;
    ll += *aStr - '0';
    aStr++;
  }

  return ll;
}

