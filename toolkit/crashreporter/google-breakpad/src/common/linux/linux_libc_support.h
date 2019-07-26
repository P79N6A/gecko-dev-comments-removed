
































#ifndef CLIENT_LINUX_LINUX_LIBC_SUPPORT_H_
#define CLIENT_LINUX_LINUX_LIBC_SUPPORT_H_

#include <stdint.h>
#include <limits.h>
#include <sys/types.h>

extern "C" {

extern size_t my_strlen(const char* s);

extern int my_strcmp(const char* a, const char* b);

extern int my_strncmp(const char* a, const char* b, size_t len);





extern bool my_strtoui(int* result, const char* s);


extern unsigned my_uint_len(uintmax_t i);







extern void my_uitos(char* output, uintmax_t i, unsigned i_len);

extern const char* my_strchr(const char* haystack, char needle);

extern const char* my_strrchr(const char* haystack, char needle);





extern const char* my_read_hex_ptr(uintptr_t* result, const char* s);

extern const char* my_read_decimal_ptr(uintptr_t* result, const char* s);

extern void my_memset(void* ip, char c, size_t len);



#define my_memcpy  memcpy
#define my_memmove memmove
#define my_memcmp  memcmp

extern size_t my_strlcpy(char* s1, const char* s2, size_t len);

extern size_t my_strlcat(char* s1, const char* s2, size_t len);

extern int my_isspace(int ch);

}  

#endif  
