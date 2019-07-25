










































#ifndef V8_PLATFORM_H_
#define V8_PLATFORM_H_


#ifdef WIN32


#ifdef _MSC_VER

enum {
  FP_NAN,
  FP_INFINITE,
  FP_ZERO,
  FP_SUBNORMAL,
  FP_NORMAL
};

int fpclassify(double x);

int strncasecmp(const char* s1, const char* s2, int n);

#endif  

#endif  



#ifdef __GNUC__


#include <stdarg.h>

#define __GNUC_VERSION__ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)

#endif  

namespace v8 {
namespace internal {

double ceiling(double x);








class OS {
 public:
  
  static void Abort();
};

} }  

#endif  
