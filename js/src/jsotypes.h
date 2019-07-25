

















































#ifndef PROTYPES_H
#define PROTYPES_H


#ifdef XP_UNIX
#include <sys/types.h>
#else
typedef JSUintn uint;
#endif

typedef JSUintn uintn;
typedef JSUint64 uint64;
typedef JSUint32 uint32;
typedef JSUint16 uint16;
typedef JSUint8 uint8;

#ifndef _XP_Core_
typedef JSIntn intn;
#endif







#if defined(AIX) && defined(HAVE_SYS_INTTYPES_H)
#include <sys/inttypes.h>
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
typedef JSInt64 int64;



typedef signed int int32;
typedef signed short int16;
typedef signed char int8;
#else
typedef JSInt64 int64;


typedef JSInt32 int32;
typedef JSInt16 int16;
typedef JSInt8 int8;
#endif 

typedef JSFloat64 float64;


#define TEST_BIT        JS_TEST_BIT
#define SET_BIT         JS_SET_BIT
#define CLEAR_BIT       JS_CLEAR_BIT

#endif 
