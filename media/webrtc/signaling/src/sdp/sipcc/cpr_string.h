



#ifndef _CPR_STRING_H_
#define _CPR_STRING_H_

#include <stdarg.h>

#include "cpr_types.h"
#include "cpr_strings.h"

#ifdef __cplusplus
extern "C" {
#endif




















unsigned long
sstrncpy(char *dst, const char *src, unsigned long max);




















char *
sstrncat(char *s1, const char *s2, unsigned long max);





#define FLEX_STRING_CHUNK_SIZE 256

typedef struct {
  char *buffer;
  size_t buffer_length;
  size_t string_length;
} flex_string;






void flex_string_init(flex_string *fs);






void flex_string_free(flex_string *fs);








void flex_string_check_alloc(flex_string *fs, size_t new_min_length);






void flex_string_append(flex_string *fs, const char *more);






void flex_string_vsprintf(flex_string *fs, const char *format, va_list original_ap);






void flex_string_sprintf(flex_string *fs, const char *format, ...);
















char *
cpr_strdup(const char *str);

#ifdef __cplusplus
}
#endif

#endif
