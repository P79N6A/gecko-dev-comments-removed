



#include <stdarg.h>

#include "mozilla/Assertions.h"
#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "cpr_strings.h"


















unsigned long
sstrncpy (char *dst, const char *src, unsigned long max)
{
    unsigned long cnt = 0;

    if (dst == NULL) {
        return 0;
    }

    if (src) {
        while ((max-- > 1) && (*src)) {
            *dst = *src;
            dst++;
            src++;
            cnt++;
        }
    }

#if defined(CPR_SSTRNCPY_PAD)
    



    while (max-- > 1) {
        *dst = '\0';
        dst++;
    }
#endif
    *dst = '\0';

    return cnt;
}

















char *
sstrncat (char *s1, const char *s2, unsigned long max)
{
    if (s1 == NULL)
        return (char *) NULL;

    while (*s1)
        s1++;

    if (s2) {
        while ((max-- > 1) && (*s2)) {
            *s1 = *s2;
            s1++;
            s2++;
        }
    }
    *s1 = '\0';

    return s1;
}










void flex_string_init(flex_string *fs) {
  fs->buffer_length = FLEX_STRING_CHUNK_SIZE;
  fs->string_length = 0;
  fs->buffer = cpr_malloc(fs->buffer_length);
  fs->buffer[0] = '\0';
}






void flex_string_free(flex_string *fs) {
  fs->buffer_length = 0;
  fs->string_length = 0;
  cpr_free(fs->buffer);
  fs->buffer = NULL;
}


#define FLEX_STRING_MAX_SIZE (10 * 1024 * 1024) /* 10MB */








void flex_string_check_alloc(flex_string *fs, size_t new_min_length) {
  if (new_min_length > fs->buffer_length) {
    

    
    if (new_min_length > FLEX_STRING_MAX_SIZE) {
      MOZ_CRASH();
    }

    
    fs->buffer_length = (((new_min_length - 1) / FLEX_STRING_CHUNK_SIZE) + 1) * FLEX_STRING_CHUNK_SIZE;

    fs->buffer = cpr_realloc(fs->buffer, fs->buffer_length);
  }
}






void flex_string_append(flex_string *fs, const char *more) {
  fs->string_length += strlen(more);

  flex_string_check_alloc(fs, fs->string_length + 1);

  sstrncat(fs->buffer, more, fs->buffer_length - strlen(fs->buffer));
}




#ifndef va_copy
#define va_copy(d,s) ((d) = (s))
#endif






void flex_string_vsprintf(flex_string *fs, const char *format, va_list original_ap) {
  va_list ap;
  int vsnprintf_result;

  va_copy(ap, original_ap);
  vsnprintf_result = vsnprintf(fs->buffer + fs->string_length, fs->buffer_length - fs->string_length, format, ap);
  va_end(ap);

  

  if (vsnprintf_result < 0) {
    va_copy(ap, original_ap);
    vsnprintf_result = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
  }

  if (fs->string_length + vsnprintf_result >= fs->buffer_length) {
    
    flex_string_check_alloc(fs, fs->string_length + vsnprintf_result + 1);

    
    va_copy(ap, original_ap);
    vsnprintf_result = vsnprintf(fs->buffer + fs->string_length, fs->buffer_length - fs->string_length, format, ap);
    va_end(ap);
    MOZ_ASSERT(vsnprintf_result > 0 && vsnprintf_result < (fs->buffer_length - fs->string_length));
  }

  if (vsnprintf_result > 0) {
    fs->string_length += vsnprintf_result;
  }
}






void flex_string_sprintf(flex_string *fs, const char *format, ...) {
  va_list ap;

  va_start(ap, format);
  flex_string_vsprintf(fs, format, ap);
  va_end(ap);
}

