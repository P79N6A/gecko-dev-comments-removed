















#ifndef ANDROID_UNICODE_H
#define ANDROID_UNICODE_H

#include <sys/types.h>
#include <stdint.h>

extern "C" {

typedef uint32_t char32_t;
typedef uint16_t char16_t;


int strcmp16(const char16_t *, const char16_t *);
int strncmp16(const char16_t *s1, const char16_t *s2, size_t n);
size_t strlen16(const char16_t *);
size_t strnlen16(const char16_t *, size_t);
char16_t *strcpy16(char16_t *, const char16_t *);
char16_t *strncpy16(char16_t *, const char16_t *, size_t);








int strzcmp16(const char16_t *s1, size_t n1, const char16_t *s2, size_t n2);


int strzcmp16_h_n(const char16_t *s1H, size_t n1, const char16_t *s2N, size_t n2);


size_t strlen32(const char32_t *);
size_t strnlen32(const char32_t *, size_t);





ssize_t utf32_to_utf8_length(const char32_t *src, size_t src_len);




































void utf32_to_utf8(const char32_t* src, size_t src_len, char* dst);








int32_t utf32_from_utf8_at(const char *src, size_t src_len, size_t index, size_t *next_index);





ssize_t utf16_to_utf8_length(const char16_t *src, size_t src_len);






void utf16_to_utf8(const char16_t* src, size_t src_len, char* dst);

















ssize_t utf8_length(const char *src);




size_t utf8_to_utf32_length(const char *src, size_t src_len);






void utf8_to_utf32(const char* src, size_t src_len, char32_t* dst);




ssize_t utf8_to_utf16_length(const uint8_t* src, size_t srcLen);






char16_t* utf8_to_utf16_no_null_terminator(const uint8_t* src, size_t srcLen, char16_t* dst);






void utf8_to_utf16(const uint8_t* src, size_t srcLen, char16_t* dst);

}

#endif
