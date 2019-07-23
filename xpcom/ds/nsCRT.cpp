




































 















#include "nsCRT.h"
#include "nsIServiceManager.h"
#include "nsCharTraits.h"

#define ADD_TO_HASHVAL(hashval, c) \
    hashval = (hashval>>28) ^ (hashval<<4) ^ (c)







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

  









PRUint32 nsCRT::HashCode(const char* str, PRUint32* resultingStrLen)
{
  PRUint32 h = 0;
  const char* s = str;

  if (!str) return h;

  unsigned char c;
  while ( (c = *s++) )
    ADD_TO_HASHVAL(h, c);

  if ( resultingStrLen )
    *resultingStrLen = (s-str)-1;
  return h;
}

PRUint32 nsCRT::HashCode(const PRUnichar* str, PRUint32* resultingStrLen)
{
  PRUint32 h = 0;
  const PRUnichar* s = str;

  if (!str) return h;

  PRUnichar c;
  while ( (c = *s++) )
    ADD_TO_HASHVAL(h, c);

  if ( resultingStrLen )
    *resultingStrLen = (s-str)-1;
  return h;
}

PRUint32 nsCRT::HashCodeAsUTF8(const PRUnichar* str, PRUint32* resultingStrLen)
{
  PRUint32 h = 0;
  const PRUnichar* s = str;

  {
    PRUint16 W1 = 0;      
    PRUint32 U = 0;       
    int code_length = 0;  

    PRUint16 W;
    while ( (W = *s++) )
      {
          





        if ( !W1 )
          {
            if ( !IS_SURROGATE(W) )
              {
                U = W;
                if ( W <= 0x007F )
                  code_length = 1;
                else if ( W <= 0x07FF )
                  code_length = 2;
                else
                  code_length = 3;
              }
            else if ( NS_IS_HIGH_SURROGATE(W) )
              W1 = W;
#ifdef DEBUG
            else NS_ERROR("Got low surrogate but no previous high surrogate");
#endif
          }
        else
          {
              
              

            if ( NS_IS_LOW_SURROGATE(W) )
              {
                U = SURROGATE_TO_UCS4(W1, W);
                NS_ASSERTION(IS_VALID_CHAR(U), "How did this happen?");
                code_length = 4;
              }
#ifdef DEBUG
            else NS_ERROR("High surrogate not followed by low surrogate");
#endif
            W1 = 0;
          }


        if ( code_length > 0 )
          {
            static const PRUint16 sBytePrefix[5]  = { 0x0000, 0x0000, 0x00C0, 0x00E0, 0x00F0  };
            static const PRUint16 sShift[5]       = { 0, 0, 6, 12, 18 };

              






              
            ADD_TO_HASHVAL(h, (sBytePrefix[code_length] |
                               (U>>sShift[code_length])));

              
            switch ( code_length )
              {  
                case 4:   ADD_TO_HASHVAL(h, (0x80 | ((U>>12) & 0x003F)));
                case 3:   ADD_TO_HASHVAL(h, (0x80 | ((U>>6 ) & 0x003F)));
                case 2:   ADD_TO_HASHVAL(h, (0x80 | ( U      & 0x003F)));
                default:  code_length = 0;
                  break;
              }
          }
      }
  }

  if ( resultingStrLen )
    *resultingStrLen = (s-str)-1;
  return h;
}

PRUint32 nsCRT::BufferHashCode(const PRUnichar* s, PRUint32 len)
{
  PRUint32 h = 0;
  const PRUnichar* done = s + len;

  while ( s < done )
    h = (h>>28) ^ (h<<4) ^ PRUint16(*s++); 

  return h;
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

