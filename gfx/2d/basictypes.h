



#ifndef BASE_BASICTYPES_H_
#define BASE_BASICTYPES_H_




#if defined(prtypes_h___) && !defined(BUILD_BUILD_CONFIG_H_)
#error You_must_include_basictypes.h_before_prtypes.h!
#endif

#ifndef NO_NSPR_10_SUPPORT
#define NO_NSPR_10_SUPPORT
#define NO_NSPR_10_SUPPORT_SAVE
#endif


#ifdef NO_NSPR_10_SUPPORT_SAVE
#undef NO_NSPR_10_SUPPORT_SAVE
#undef NO_NSPR_10_SUPPORT
#endif

#ifdef _WIN32
#undef _WIN32
#define _WIN32_SAVE
#endif


#ifdef _WIN32_SAVE
#undef _WIN32_SAVE
#define _WIN32
#endif

#include <limits.h>         
#include <stddef.h>         
#include <string.h>         



#ifndef COMPILER_MSVC

#include <stdint.h>         
#endif
typedef uint8_t uint8;
typedef int16_t int16;
#if 0




typedef signed int         char32;

const uint8  kuint8max  = (( uint8) 0xFF);
const uint16 kuint16max = ((uint16) 0xFFFF);
const uint32 kuint32max = ((uint32) 0xFFFFFFFF);
const uint64 kuint64max = ((uint64) GG_LONGLONG(0xFFFFFFFFFFFFFFFF));
const  int8  kint8min   = ((  int8) 0x80);
const  int8  kint8max   = ((  int8) 0x7F);
const  int16 kint16min  = (( int16) 0x8000);
const  int16 kint16max  = (( int16) 0x7FFF);
const  int32 kint32min  = (( int32) 0x80000000);
const  int32 kint32max  = (( int32) 0x7FFFFFFF);
const  int64 kint64min  = (( int64) GG_LONGLONG(0x8000000000000000));
const  int64 kint64max  = (( int64) GG_LONGLONG(0x7FFFFFFFFFFFFFFF));
#endif

#  if defined(OS_POSIX)
#    define __STDC_FORMAT_MACROS 1
#    include <inttypes.h>           
#    define PRId64L "I64d"
#    define PRIu64L "I64u"
#    define PRIx64L "I64x"
#  elif defined(OS_WIN)
#    define PRId64 "I64d"
#    define PRIu64 "I64u"
#    define PRIx64 "I64x"
#    define PRId64L L"I64d"
#    define PRIu64L L"I64u"
#    define PRIx64L L"I64x"
#  endif



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
















template <bool>
struct CompileAssert {
};

#undef COMPILE_ASSERT
#define COMPILE_ASSERT(expr, msg) \
  typedef CompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]


















































enum Ownership {
  DO_NOT_TAKE_OWNERSHIP,
  TAKE_OWNERSHIP
};























































template <class Dest, class Source>
inline Dest bit_cast(const Source& source) {
  
  
  typedef char VerifySizesAreEqual [sizeof(Dest) == sizeof(Source) ? 1 : -1];

  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}














namespace base {
enum LinkerInitialized { LINKER_INITIALIZED };
}  




#endif  
