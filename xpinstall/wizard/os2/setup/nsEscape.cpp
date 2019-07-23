








































#ifdef __cplusplus
extern "C"
{
#endif

#include "extern.h"
#include "extra.h"

#ifdef __cplusplus
}
#endif 

#include "nsEscape.h"

typedef int PRInt32;

const int netCharType[256] =





    
    {    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	
		 0,0,0,0,0,0,0,0,0,0,7,4,0,7,7,4,	
         7,7,7,7,7,7,7,7,7,7,0,0,0,7,0,0,	
	     0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,	
	     
	     
	     7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,	
	     0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,	
	     7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,	
		 0, };



#define UNHEX(C) \
    ((C >= '0' && C <= '9') ? C - '0' : \
     ((C >= 'A' && C <= 'F') ? C - 'A' + 10 : \
     ((C >= 'a' && C <= 'f') ? C - 'a' + 10 : 0)))


#define IS_OK(C) (netCharType[((unsigned int) (C))] & (mask))
#define HEX_ESCAPE '%'


NS_COM char* nsEscape(const char * str, nsEscapeMask mask)

{
    if(!str)
        return NULL;
    return nsEscapeCount(str, strlen(str), mask, NULL);
}


NS_COM char* nsEscapeCount(
    const char * str,
    PRInt32 len,
    nsEscapeMask mask,
    PRInt32* out_len)

{
	if (!str)
		return 0;

    int i, extra = 0;
    char* hexChars = "0123456789ABCDEF";

	register const unsigned char* src = (const unsigned char *) str;
    for (i = 0; i < len; i++)
	{
        if (!IS_OK(*src++))
            extra += 2; 
	}

	char* result = (char *)NS_GlobalAlloc(len + extra + 1);
    if (!result)
        return 0;

    register unsigned char* dst = (unsigned char *) result;
	src = (const unsigned char *) str;
	if (mask == url_XPAlphas)
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


NS_COM char* nsUnescape(char * str)

{
	nsUnescapeCount(str);
	return str;
}


NS_COM PRInt32 nsUnescapeCount(char * str)

{
    register char *src = str;
    register char *dst = str;

    while (*src)
        if (*src != HEX_ESCAPE)
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

    *dst = 0;
    return (int)(dst - str);

} 

