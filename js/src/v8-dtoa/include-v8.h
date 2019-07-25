


























#ifndef V8_H_
#define V8_H_





#include <stdio.h>

#ifdef _WIN32

#ifdef __MINGW32__
#include <stdint.h>
#else  
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;  
typedef unsigned short uint16_t;  
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#endif  

#else  

#include <stdint.h>

#endif  

#endif  
