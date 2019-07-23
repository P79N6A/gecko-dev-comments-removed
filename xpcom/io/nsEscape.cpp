



































 


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
    PRInt32 len,
    nsEscapeMask flags,
    PRInt32* out_len)

{
	if (!str)
		return 0;

    int i, extra = 0;
    static const char hexChars[] = "0123456789ABCDEF";

	register const unsigned char* src = (const unsigned char *) str;
    for (i = 0; i < len; i++)
	{
        if (!IS_OK(*src++))
            extra += 2; 
	}

	char* result = (char *)nsMemory::Alloc(len + extra + 1);
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


NS_COM char* nsEscape(const char * str, nsEscapeMask flags)

{
    if(!str)
        return NULL;
    return nsEscapeCount(str, (PRInt32)strlen(str), flags, NULL);
}


NS_COM char* nsUnescape(char * str)

{
	nsUnescapeCount(str);
	return str;
}


NS_COM PRInt32 nsUnescapeCount(char * str)

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


NS_COM char *
nsEscapeHTML(const char * string)
{
	
	char *rv = (char *) nsMemory::Alloc(strlen(string) * 6 + 1);
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

NS_COM PRUnichar *
nsEscapeHTML2(const PRUnichar *aSourceBuffer, PRInt32 aSourceBufferLen)
{
  
  if (aSourceBufferLen == -1) {
    aSourceBufferLen = nsCRT::strlen(aSourceBuffer); 
  }

  
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
     1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1008, 912,   0,1008,   0, 768,       
     1008,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,       
     1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896, 896, 896, 896,1023,       
        0,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,       
     1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896,1012, 896,1023,   0,       
        0    
};

#define NO_NEED_ESC(C) (EscapeChars[((unsigned int) (C))] & (flags))





























NS_COM PRBool NS_EscapeURL(const char *part,
                           PRInt32 partLen,
                           PRUint32 flags,
                           nsACString &result)
{
    if (!part) {
        NS_NOTREACHED("null pointer");
        return PR_FALSE;
    }

    int i = 0;
    static const char hexChars[] = "0123456789ABCDEF";
    if (partLen < 0)
        partLen = strlen(part);
    PRBool forced = (flags & esc_Forced);
    PRBool ignoreNonAscii = (flags & esc_OnlyASCII);
    PRBool ignoreAscii = (flags & esc_OnlyNonASCII);
    PRBool writing = (flags & esc_AlwaysCopy);
    PRBool colon = (flags & esc_Colon);

    register const unsigned char* src = (const unsigned char *) part;

    char tempBuffer[100];
    unsigned int tempBufferPos = 0;

    PRBool previousIsNonASCII = PR_FALSE;
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
          writing = PR_TRUE;
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

NS_COM PRBool NS_UnescapeURL(const char *str, PRInt32 len, PRUint32 flags, nsACString &result)
{
    if (!str) {
        NS_NOTREACHED("null pointer");
        return PR_FALSE;
    }

    if (len < 0)
        len = strlen(str);

    PRBool ignoreNonAscii = (flags & esc_OnlyASCII);
    PRBool ignoreAscii = (flags & esc_OnlyNonASCII);
    PRBool writing = (flags & esc_AlwaysCopy);
    PRBool skipControl = (flags & esc_SkipControl); 

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
                
                writing = PR_TRUE;
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
