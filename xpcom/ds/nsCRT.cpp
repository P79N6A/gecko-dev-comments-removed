




































 















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
  PRUint32 i;
  char* result;
  char* str = string;

  for (i = 0; i < DELIM_TABLE_SIZE; i++)
    delimTable[i] = '\0';

  for (i = 0; delims[i]; i++) {
    SET_DELIM(delimTable, static_cast<PRUint8>(delims[i]));
  }
  NS_ASSERTION(delims[i] == '\0', "too many delimiters");

  
  while (*str && IS_DELIM(delimTable, static_cast<PRUint8>(*str))) {
    str++;
  }
  result = str;

  
  while (*str) {
    if (IS_DELIM(delimTable, static_cast<PRUint8>(*str))) {
      *str++ = '\0';
      break;
    }
    str++;
  }
  *newStr = str;

  return str == result ? NULL : result;
}












PRInt32 nsCRT::strcmp(const PRUnichar* s1, const PRUnichar* s2) {
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










PRInt32 nsCRT::strncmp(const PRUnichar* s1, const PRUnichar* s2, PRUint32 n) {
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

PRUnichar* nsCRT::strdup(const PRUnichar* str)
{
  PRUint32 len = nsCRT::strlen(str);
  return strndup(str, len);
}

PRUnichar* nsCRT::strndup(const PRUnichar* str, PRUint32 len)
{
	nsCppSharedAllocator<PRUnichar> shared_allocator;
	PRUnichar* rslt = shared_allocator.allocate(len + 1); 
  

  if (rslt == NULL) return NULL;
  memcpy(rslt, str, len * sizeof(PRUnichar));
  rslt[len] = 0;
  return rslt;
}



PRInt64 nsCRT::atoll(const char *str)
{
    if (!str)
        return LL_Zero();

    PRInt64 ll = LL_Zero(), digitll = LL_Zero();

    while (*str && *str >= '0' && *str <= '9') {
        LL_MUL(ll, ll, 10);
        LL_UI2L(digitll, (*str - '0'));
        LL_ADD(ll, ll, digitll);
        str++;
    }

    return ll;
}

