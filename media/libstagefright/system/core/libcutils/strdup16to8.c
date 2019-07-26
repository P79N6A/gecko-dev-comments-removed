
















#include <limits.h>  

#include <cutils/jstring.h>
#include <assert.h>
#include <stdlib.h>






extern size_t strnlen16to8(const char16_t* utf16Str, size_t len)
{
    size_t utf8Len = 0;

    





















    

    if (len < (SIZE_MAX-1)/3) {
        while (len--) {
            unsigned int uic = *utf16Str++;

            if (uic > 0x07ff)
                utf8Len += 3;
            else if (uic > 0x7f || uic == 0)
                utf8Len += 2;
            else
                utf8Len++;
        }
        return utf8Len;
    }

    
    while (len--) {
        unsigned int  uic     = *utf16Str++;
        size_t        utf8Cur = utf8Len;

        if (uic > 0x07ff)
            utf8Len += 3;
        else if (uic > 0x7f || uic == 0)
            utf8Len += 2;
        else
            utf8Len++;

        if (utf8Len < utf8Cur) 
            return SIZE_MAX-1;
    }

    
    if (utf8Len == SIZE_MAX)
        utf8Len = SIZE_MAX-1;

    return utf8Len;
}














extern char* strncpy16to8(char* utf8Str, const char16_t* utf16Str, size_t len)
{
    char* utf8cur = utf8Str;

    



    while (len--) {
        unsigned int uic = *utf16Str++;

        if (uic > 0x07ff) {
            *utf8cur++ = (uic >> 12) | 0xe0;
            *utf8cur++ = ((uic >> 6) & 0x3f) | 0x80;
            *utf8cur++ = (uic & 0x3f) | 0x80;
        } else if (uic > 0x7f || uic == 0) {
            *utf8cur++ = (uic >> 6) | 0xc0;
            *utf8cur++ = (uic & 0x3f) | 0x80;
        } else {
            *utf8cur++ = uic;

            if (uic == 0) {
                break;
            }
        }
    }

   *utf8cur = '\0';

   return utf8Str;
}





char * strndup16to8 (const char16_t* s, size_t n)
{
    char*   ret;
    size_t  len;

    if (s == NULL) {
        return NULL;
    }

    len = strnlen16to8(s, n);

    



    if (len >= SIZE_MAX-1)
        return NULL;

    ret = malloc(len + 1);
    if (ret == NULL)
        return NULL;

    strncpy16to8 (ret, s, n);

    return ret;
}
