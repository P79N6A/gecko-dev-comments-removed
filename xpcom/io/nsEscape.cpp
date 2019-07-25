



































 


#include "nsEscape.h"
#include "nsMemory.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"

const int netCharType[256] =





    
    {    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	
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


#define IS_OK(C) (netCharType[((unsigned int) (C))] & (flags))
#define HEX_ESCAPE '%'


static char* nsEscapeCount(
    const char * str,
    nsEscapeMask flags,
    size_t* out_len)

{
	if (!str)
		return 0;

    size_t i, len = 0, charsToEscape = 0;
    static const char hexChars[] = "0123456789ABCDEF";

	register const unsigned char* src = (const unsigned char *) str;
    while (*src)
	{
        len++;
        if (!IS_OK(*src++))
            charsToEscape++;
	}

    
    
    
    size_t dstSize = len + 1 + charsToEscape;
    if (dstSize <= len)
	return 0;
    dstSize += charsToEscape;
    if (dstSize < len)
	return 0;

    
    
    
    
    if (dstSize > PR_UINT32_MAX)
        return 0;

	char* result = (char *)nsMemory::Alloc(dstSize);
    if (!result)
        return 0;

    register unsigned char* dst = (unsigned char *) result;
	src = (const unsigned char *) str;
	if (flags == url_XPAlphas)
	{
	    for (i = 0; i < len; i++)
		{
			unsigned char c = *src++;
			if (IS_OK(c))
				*dst++ = c;
			else if (c == ' ')
				*dst++ = '+'; 
			else 
			{
				*dst++ = HEX_ESCAPE;
				*dst++ = hexChars[c >> 4];	
				*dst++ = hexChars[c & 0x0f];	
			}
		}
	}
	else
	{
	    for (i = 0; i < len; i++)
		{
			unsigned char c = *src++;
			if (IS_OK(c))
				*dst++ = c;
			else 
			{
				*dst++ = HEX_ESCAPE;
				*dst++ = hexChars[c >> 4];	
				*dst++ = hexChars[c & 0x0f];	
			}
		}
	}

    *dst = '\0';     
	if(out_len)
		*out_len = dst - (unsigned char *) result;
    return result;
}


char* nsEscape(const char * str, nsEscapeMask flags)

{
    if(!str)
        return NULL;
    return nsEscapeCount(str, flags, NULL);
}


char* nsUnescape(char * str)

{
	nsUnescapeCount(str);
	return str;
}


PRInt32 nsUnescapeCount(char * str)

{
    register char *src = str;
    register char *dst = str;
    static const char hexChars[] = "0123456789ABCDEFabcdef";

    char c1[] = " ";
    char c2[] = " ";
    char* const pc1 = c1;
    char* const pc2 = c2;

    while (*src)
    {
        c1[0] = *(src+1);
        if (*(src+1) == '\0') 
            c2[0] = '\0';
        else
            c2[0] = *(src+2);

        if (*src != HEX_ESCAPE || PL_strpbrk(pc1, hexChars) == 0 || 
                                  PL_strpbrk(pc2, hexChars) == 0 )
        	*dst++ = *src++;
        else 	
		{
        	src++; 
        	if (*src)
            {
            	*dst = UNHEX(*src) << 4;
            	src++;
            }
        	if (*src)
            {
            	*dst = (*dst + UNHEX(*src));
            	src++;
            }
        	dst++;
        }
    }

    *dst = 0;
    return (int)(dst - str);

} 


char *
nsEscapeHTML(const char * string)
{
    char *rv = nsnull;
    
    PRUint32 len = PL_strlen(string);
    if (len >= (PR_UINT32_MAX / 6))
      return nsnull;

    rv = (char *)NS_Alloc( (6 * len) + 1 );
    char *ptr = rv;

    if(rv)
      {
        for(; *string != '\0'; string++)
          {
            if(*string == '<')
              {
                *ptr++ = '&';
                *ptr++ = 'l';
                *ptr++ = 't';
                *ptr++ = ';';
              }
            else if(*string == '>')
              {
                *ptr++ = '&';
                *ptr++ = 'g';
                *ptr++ = 't';
                *ptr++ = ';';
              }
            else if(*string == '&')
              {
                *ptr++ = '&';
                *ptr++ = 'a';
                *ptr++ = 'm';
                *ptr++ = 'p';
                *ptr++ = ';';
              }
            else if (*string == '"')
              {
                *ptr++ = '&';
                *ptr++ = 'q';
                *ptr++ = 'u';
                *ptr++ = 'o';
                *ptr++ = 't';
                *ptr++ = ';';
              }
            else if (*string == '\'')
              {
                *ptr++ = '&';
                *ptr++ = '#';
                *ptr++ = '3';
                *ptr++ = '9';
                *ptr++ = ';';
              }
            else
              {
                *ptr++ = *string;
              }
          }
        *ptr = '\0';
      }

    return(rv);
}

PRUnichar *
nsEscapeHTML2(const PRUnichar *aSourceBuffer, PRInt32 aSourceBufferLen)
{
  
  if (aSourceBufferLen < 0) {
    aSourceBufferLen = nsCRT::strlen(aSourceBuffer); 
  }

  
  if (PRUint32(aSourceBufferLen) >=
      ((PR_UINT32_MAX - sizeof(PRUnichar)) / (6 * sizeof(PRUnichar))) )
    return nsnull;

  PRUnichar *resultBuffer = (PRUnichar *)nsMemory::Alloc(aSourceBufferLen *
                            6 * sizeof(PRUnichar) + sizeof(PRUnichar('\0')));
  PRUnichar *ptr = resultBuffer;

  if (resultBuffer) {
    PRInt32 i;

    for(i = 0; i < aSourceBufferLen; i++) {
      if(aSourceBuffer[i] == '<') {
        *ptr++ = '&';
        *ptr++ = 'l';
        *ptr++ = 't';
        *ptr++ = ';';
      } else if(aSourceBuffer[i] == '>') {
        *ptr++ = '&';
        *ptr++ = 'g';
        *ptr++ = 't';
        *ptr++ = ';';
      } else if(aSourceBuffer[i] == '&') {
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
     1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896, 896, 896, 896,1023,       
        0,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,       
     1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896,1012, 896,1023,   0,       
        0    
};

#define NO_NEED_ESC(C) (EscapeChars[((unsigned int) (C))] & (flags))





























bool NS_EscapeURL(const char *part,
                           PRInt32 partLen,
                           PRUint32 flags,
                           nsACString &result)
{
    if (!part) {
        NS_NOTREACHED("null pointer");
        return false;
    }

    int i = 0;
    static const char hexChars[] = "0123456789ABCDEF";
    if (partLen < 0)
        partLen = strlen(part);
    bool forced = !!(flags & esc_Forced);
    bool ignoreNonAscii = !!(flags & esc_OnlyASCII);
    bool ignoreAscii = !!(flags & esc_OnlyNonASCII);
    bool writing = !!(flags & esc_AlwaysCopy);
    bool colon = !!(flags & esc_Colon);

    register const unsigned char* src = (const unsigned char *) part;

    char tempBuffer[100];
    unsigned int tempBufferPos = 0;

    bool previousIsNonASCII = false;
    for (i = 0; i < partLen; i++)
    {
      unsigned char c = *src++;

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if ((NO_NEED_ESC(c) || (c == HEX_ESCAPE && !forced)
                          || (c > 0x7f && ignoreNonAscii)
                          || (c > 0x20 && c < 0x7f && ignoreAscii))
          && !(c == ':' && colon)
          && !(previousIsNonASCII && c == '|' && !ignoreNonAscii))
      {
        if (writing)
          tempBuffer[tempBufferPos++] = c;
      }
      else 
      {
        if (!writing)
        {
          result.Append(part, i);
          writing = true;
        }
        tempBuffer[tempBufferPos++] = HEX_ESCAPE;
        tempBuffer[tempBufferPos++] = hexChars[c >> 4];	
        tempBuffer[tempBufferPos++] = hexChars[c & 0x0f]; 
      }

      if (tempBufferPos >= sizeof(tempBuffer) - 4)
      {
        NS_ASSERTION(writing, "should be writing");
        tempBuffer[tempBufferPos] = '\0';
        result += tempBuffer;
        tempBufferPos = 0;
      }

      previousIsNonASCII = (c > 0x7f);
    }
    if (writing) {
      tempBuffer[tempBufferPos] = '\0';
      result += tempBuffer;
    }
    return writing;
}

#define ISHEX(c) memchr(hexChars, c, sizeof(hexChars)-1)

bool NS_UnescapeURL(const char *str, PRInt32 len, PRUint32 flags, nsACString &result)
{
    if (!str) {
        NS_NOTREACHED("null pointer");
        return false;
    }

    if (len < 0)
        len = strlen(str);

    bool ignoreNonAscii = !!(flags & esc_OnlyASCII);
    bool ignoreAscii = !!(flags & esc_OnlyNonASCII);
    bool writing = !!(flags & esc_AlwaysCopy);
    bool skipControl = !!(flags & esc_SkipControl); 

    static const char hexChars[] = "0123456789ABCDEFabcdef";

    const char *last = str;
    const char *p = str;

    for (int i=0; i<len; ++i, ++p) {
        
        if (*p == HEX_ESCAPE && i < len-2) {
            unsigned char *p1 = ((unsigned char *) p) + 1;
            unsigned char *p2 = ((unsigned char *) p) + 2;
            if (ISHEX(*p1) && ISHEX(*p2) && 
                ((*p1 < '8' && !ignoreAscii) || (*p1 >= '8' && !ignoreNonAscii)) &&
                !(skipControl && 
                  (*p1 < '2' || (*p1 == '7' && (*p2 == 'f' || *p2 == 'F'))))) {
                
                writing = true;
                if (p > last) {
                    
                    result.Append(last, p - last);
                    last = p;
                }
                char u = (UNHEX(*p1) << 4) + UNHEX(*p2);
                
                result.Append(u);
                i += 2;
                p += 2;
                last += 3;
            }
        }
    }
    if (writing && last < str + len)
        result.Append(last, str + len - last);

    return writing;
}
