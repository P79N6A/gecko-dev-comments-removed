

























#pragma once

#include <stddef.h>

typedef unsigned char   gr_uint8;
typedef gr_uint8        gr_byte;
typedef signed char     gr_int8;
typedef unsigned short  gr_uint16;
typedef short           gr_int16;
typedef unsigned int    gr_uint32;
typedef int             gr_int32;

enum gr_encform {
  gr_utf8 = 1, gr_utf16 = 2, gr_utf32 = 4
};


#if defined _WIN32 || defined __CYGWIN__
  #ifdef GR2_EXPORTING
    #ifdef __GNUC__
      #define GR2_API    __attribute__((dllexport))
    #else
      #define GR2_API    __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define GR2_API    __attribute__((dllimport))
    #else
      #define GR2_API    __declspec(dllimport)
    #endif
  #endif
  #define GR2_LOCAL
#else
  #if __GNUC__ >= 4
    #define GR2_API      __attribute__ ((visibility("default")))
    #define GR2_LOCAL       __attribute__ ((visibility("hidden")))
  #else
    #define GR2_API
    #define GR2_LOCAL
  #endif
#endif
