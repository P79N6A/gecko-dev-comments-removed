



















#ifndef AVUTIL_AVSTRING_H
#define AVUTIL_AVSTRING_H

#include <stddef.h>
#include "attributes.h"















int av_strstart(const char *str, const char *pfx, const char **ptr);











int av_stristart(const char *str, const char *pfx, const char **ptr);













char *av_stristr(const char *haystack, const char *needle);














char *av_strnstr(const char *haystack, const char *needle, size_t hay_length);
















size_t av_strlcpy(char *dst, const char *src, size_t size);

















size_t av_strlcat(char *dst, const char *src, size_t size);













size_t av_strlcatf(char *dst, size_t size, const char *fmt, ...) av_printf_format(3, 4);




char *av_d2str(double d);















char *av_get_token(const char **buf, const char *term);




int av_isdigit(int c);




int av_isgraph(int c);




int av_isspace(int c);




static inline int av_toupper(int c)
{
    if (c >= 'a' && c <= 'z')
        c ^= 0x20;
    return c;
}




static inline int av_tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
        c ^= 0x20;
    return c;
}




int av_isxdigit(int c);





int av_strcasecmp(const char *a, const char *b);





int av_strncasecmp(const char *a, const char *b, size_t n);







const char *av_basename(const char *path);







const char *av_dirname(char *path);








int av_match_name(const char *name, const char *names);





#endif 
