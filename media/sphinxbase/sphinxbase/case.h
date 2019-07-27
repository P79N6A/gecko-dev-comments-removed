







































































#ifndef _LIBUTIL_CASE_H_
#define _LIBUTIL_CASE_H_

#include <string.h>

#include <sphinxbase/prim_type.h>
#include <sphinxbase/sphinxbase_export.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

  


#define UPPER_CASE(c)	((((c) >= 'a') && ((c) <= 'z')) ? (c-32) : c)

  


#define LOWER_CASE(c)	((((c) >= 'A') && ((c) <= 'Z')) ? (c+32) : c)


  



SPHINXBASE_EXPORT
void ucase(char *str);

  



SPHINXBASE_EXPORT
void lcase(char *str);

  






SPHINXBASE_EXPORT
int32 strcmp_nocase(const char *str1, const char *str2);




SPHINXBASE_EXPORT
int32 strncmp_nocase(const char *str1, const char *str2, size_t len);


#ifdef __cplusplus
}
#endif

#endif
