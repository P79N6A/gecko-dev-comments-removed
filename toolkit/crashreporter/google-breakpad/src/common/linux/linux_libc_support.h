
































#ifndef CLIENT_LINUX_LINUX_LIBC_SUPPORT_H_
#define CLIENT_LINUX_LINUX_LIBC_SUPPORT_H_

#include <stdint.h>
#include <limits.h>
#include <sys/types.h>

extern "C" {

static inline size_t
my_strlen(const char* s) {
  size_t len = 0;
  while (*s++) len++;
  return len;
}

static inline int
my_strcmp(const char* a, const char* b) {
  for (;;) {
    if (*a < *b)
      return -1;
    else if (*a > *b)
      return 1;
    else if (*a == 0)
      return 0;
    a++;
    b++;
  }
}

static inline int
my_strncmp(const char* a, const char* b, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (*a < *b)
      return -1;
    else if (*a > *b)
      return 1;
    else if (*a == 0)
      return 0;
    a++;
    b++;
  }

  return 0;
}





static inline bool
my_strtoui(int* result, const char* s) {
  if (*s == 0)
    return false;
  int r = 0;
  for (;; s++) {
    if (*s == 0)
      break;
    const int old_r = r;
    r *= 10;
    if (*s < '0' || *s > '9')
      return false;
    r += *s - '0';
    if (r < old_r)
      return false;
  }

  *result = r;
  return true;
}



static inline unsigned
my_int_len(int i) {
  if (!i)
    return 1;

  int len = 0;
  while (i) {
    len++;
    i /= 10;
  }

  return len;
}







static inline void
my_itos(char* output, int i, unsigned i_len) {
  for (unsigned index = i_len; index; --index, i /= 10)
    output[index - 1] = '0' + (i % 10);
}

static inline const char*
my_strchr(const char* haystack, char needle) {
  while (*haystack && *haystack != needle)
    haystack++;
  if (*haystack == needle)
    return haystack;
  return (const char*) 0;
}





static inline const char*
my_read_hex_ptr(uintptr_t* result, const char* s) {
  uintptr_t r = 0;

  for (;; ++s) {
    if (*s >= '0' && *s <= '9') {
      r <<= 4;
      r += *s - '0';
    } else if (*s >= 'a' && *s <= 'f') {
      r <<= 4;
      r += (*s - 'a') + 10;
    } else if (*s >= 'A' && *s <= 'F') {
      r <<= 4;
      r += (*s - 'A') + 10;
    } else {
      break;
    }
  }

  *result = r;
  return s;
}

static inline void
my_memset(void* ip, char c, size_t len) {
  char* p = (char *) ip;
  while (len--)
    *p++ = c;
}

}  

#endif  
