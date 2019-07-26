
































#include "common/linux/linux_libc_support.h"

#include <stddef.h>

extern "C" {

size_t my_strlen(const char* s) {
  size_t len = 0;
  while (*s++) len++;
  return len;
}

int my_strcmp(const char* a, const char* b) {
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

int my_strncmp(const char* a, const char* b, size_t len) {
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





bool my_strtoui(int* result, const char* s) {
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


unsigned my_uint_len(uintmax_t i) {
  if (!i)
    return 1;

  int len = 0;
  while (i) {
    len++;
    i /= 10;
  }

  return len;
}







void my_uitos(char* output, uintmax_t i, unsigned i_len) {
  for (unsigned index = i_len; index; --index, i /= 10)
    output[index - 1] = '0' + (i % 10);
}

const char* my_strchr(const char* haystack, char needle) {
  while (*haystack && *haystack != needle)
    haystack++;
  if (*haystack == needle)
    return haystack;
  return (const char*) 0;
}

const char* my_strrchr(const char* haystack, char needle) {
  const char* ret = NULL;
  while (*haystack) {
    if (*haystack == needle)
      ret = haystack;
    haystack++;
  }
  return ret;
}





const char* my_read_hex_ptr(uintptr_t* result, const char* s) {
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

const char* my_read_decimal_ptr(uintptr_t* result, const char* s) {
  uintptr_t r = 0;

  for (;; ++s) {
    if (*s >= '0' && *s <= '9') {
      r *= 10;
      r += *s - '0';
    } else {
      break;
    }
  }
  *result = r;
  return s;
}

void my_memset(void* ip, char c, size_t len) {
  char* p = (char *) ip;
  while (len--)
    *p++ = c;
}

size_t my_strlcpy(char* s1, const char* s2, size_t len) {
  size_t pos1 = 0;
  size_t pos2 = 0;

  while (s2[pos2] != '\0') {
    if (pos1 + 1 < len) {
      s1[pos1] = s2[pos2];
      pos1++;
    }
    pos2++;
  }
  if (len > 0)
    s1[pos1] = '\0';

  return pos2;
}

size_t my_strlcat(char* s1, const char* s2, size_t len) {
  size_t pos1 = 0;

  while (pos1 < len && s1[pos1] != '\0')
    pos1++;

  if (pos1 == len)
    return pos1;

  return pos1 + my_strlcpy(s1 + pos1, s2, len - pos1);
}

int my_isspace(int ch) {
  
  const char spaces[] = " \t\f\n\r\t\v";
  for (size_t i = 0; i < sizeof(spaces); i++) {
    if (ch == spaces[i])
      return 1;
  }
  return 0;
}

}  
