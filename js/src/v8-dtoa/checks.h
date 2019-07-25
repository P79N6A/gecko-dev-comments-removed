


























#ifndef V8_CHECKS_H_
#define V8_CHECKS_H_

#include <string.h>

extern "C" void V8_Fatal(const char* file, int line, const char* format, ...);




#ifdef DEBUG
#define FATAL(msg)                              \
  V8_Fatal(__FILE__, __LINE__, "%s", (msg))
#define UNIMPLEMENTED()                         \
  V8_Fatal(__FILE__, __LINE__, "unimplemented code")
#define UNREACHABLE()                           \
  V8_Fatal(__FILE__, __LINE__, "unreachable code")
#else
#define FATAL(msg)                              \
  V8_Fatal("", 0, "%s", (msg))
#define UNIMPLEMENTED()                         \
  V8_Fatal("", 0, "unimplemented code")
#define UNREACHABLE() ((void) 0)
#endif


static inline void CheckHelper(const char* file,
                               int line,
                               const char* source,
                               bool condition) {
  if (!condition)
    V8_Fatal(file, line, source);
}




#define CHECK(condition) CheckHelper(__FILE__, __LINE__, #condition, condition)




static inline void CheckEqualsHelper(const char* file, int line,
                                     const char* expected_source, int expected,
                                     const char* value_source, int value) {
  if (expected != value) {
    V8_Fatal(file, line,
             "CHECK_EQ(%s, %s) failed\n#   Expected: %i\n#   Found: %i",
             expected_source, value_source, expected, value);
  }
}


#define CHECK_EQ(expected, value) CheckEqualsHelper(__FILE__, __LINE__, \
  #expected, expected, #value, value)




#ifdef DEBUG
#define ASSERT(condition)    CHECK(condition)
#else
#define ASSERT(condition)      ((void) 0)
#endif

#endif  
