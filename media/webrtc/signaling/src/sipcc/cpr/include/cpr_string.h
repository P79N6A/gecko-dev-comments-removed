



#ifndef _CPR_STRING_H_
#define _CPR_STRING_H_

#include <stdarg.h>

#include "cpr_types.h"
#include "cpr_strings.h"

__BEGIN_DECLS

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_string.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_string.h"
#define cpr_strdup _strdup
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_string.h"
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

__END_DECLS

#endif
