



#ifndef BASE_BASICTYPES_H_
#define BASE_BASICTYPES_H_

#include <limits.h>         
#include <stddef.h>         
#include <string.h>         

#include "base/port.h"    

#include "mozilla/Assertions.h"
#include "mozilla/IntegerPrintfMacros.h"





typedef signed int         char32;

const uint8_t  kuint8max  = (( uint8_t) 0xFF);
const uint16_t kuint16max = ((uint16_t) 0xFFFF);
const uint32_t kuint32max = ((uint32_t) 0xFFFFFFFF);
const uint64_t kuint64max = ((uint64_t) GG_LONGLONG(0xFFFFFFFFFFFFFFFF));
const  int8_t  kint8min   = ((  int8_t) 0x80);
const  int8_t  kint8max   = ((  int8_t) 0x7F);
const  int16_t kint16min  = (( int16_t) 0x8000);
const  int16_t kint16max  = (( int16_t) 0x7FFF);
const  int32_t kint32min  = (( int32_t) 0x80000000);
const  int32_t kint32max  = (( int32_t) 0x7FFFFFFF);
const  int64_t kint64min  = (( int64_t) GG_LONGLONG(0x8000000000000000));
const  int64_t kint64max  = (( int64_t) GG_LONGLONG(0x7FFFFFFFFFFFFFFF));


#  if defined(OS_POSIX)
#    define PRId64L "I64d"
#    define PRIu64L "I64u"
#    define PRIx64L "I64x"
#  elif defined(OS_WIN)
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

















#if !defined(COMPILE_ASSERT)
template <bool>
struct CompileAssert {
};

#define COMPILE_ASSERT(expr, msg) \
  typedef CompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]
#endif













































typedef uint32_t MetatagId;




enum Ownership {
  DO_NOT_TAKE_OWNERSHIP,
  TAKE_OWNERSHIP
};














namespace base {
enum LinkerInitialized { LINKER_INITIALIZED };
}  


#include "nscore.h"             


#endif  
