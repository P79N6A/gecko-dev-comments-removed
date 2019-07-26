















#ifndef __CUTILS_STRING16_H
#define __CUTILS_STRING16_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t char16_t;

extern char * strndup16to8 (const char16_t* s, size_t n);
extern size_t strnlen16to8 (const char16_t* s, size_t n);
extern char * strncpy16to8 (char *dest, const char16_t*s, size_t n);

extern char16_t * strdup8to16 (const char* s, size_t *out_len);
extern size_t strlen8to16 (const char* utf8Str);
extern char16_t * strcpy8to16 (char16_t *dest, const char*s, size_t *out_len);
extern char16_t * strcpylen8to16 (char16_t *dest, const char*s, int length,
    size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif
