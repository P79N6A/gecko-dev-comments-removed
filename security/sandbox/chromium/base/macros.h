








#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

#include <stddef.h>  
#include <string.h>  

#include "base/compiler_specific.h"  


#define DISALLOW_COPY(TypeName) \
  TypeName(const TypeName&)


#define DISALLOW_ASSIGN(TypeName) \
  void operator=(const TypeName&)



#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)






#define DISALLOW_EVIL_CONSTRUCTORS(TypeName) DISALLOW_COPY_AND_ASSIGN(TypeName)







#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_COPY_AND_ASSIGN(TypeName)















template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];




#ifndef _MSC_VER
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];
#endif

#define arraysize(array) (sizeof(ArraySizeHelper(array)))






































#define ARRAYSIZE_UNSAFE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))



















template<typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}
















#undef COMPILE_ASSERT

#if __cplusplus >= 201103L


#define COMPILE_ASSERT(expr, msg) static_assert(expr, #msg)

#else

template <bool>
struct CompileAssert {
};

#define COMPILE_ASSERT(expr, msg) \
  typedef CompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1] ALLOW_UNUSED










































#endif























































template <class Dest, class Source>
inline Dest bit_cast(const Source& source) {
  COMPILE_ASSERT(sizeof(Dest) == sizeof(Source), VerifySizesAreEqual);

  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}









template<typename T>
inline void ignore_result(const T&) {
}














namespace base {
enum LinkerInitialized { LINKER_INITIALIZED };




#define CR_DEFINE_STATIC_LOCAL(type, name, arguments) \
  static type& name = *new type arguments

}  

#endif  
