



#ifndef _CPR_WIN_TYPES_H_
#define _CPR_WIN_TYPES_H_

#include <sys/types.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#ifdef SIPCC_BUILD
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windef.h>
#endif
#include <stddef.h>
#include <stdlib.h>






#ifdef _MSC_VER
#define CPR_WIN32_SDK_MICROSOFT
#endif
#ifdef __GNUC__
#define CPR_WIN32_SDK_MINGW
#endif








#if defined(CPR_WIN32_SDK_MINGW)
#include <stdint.h>
#elif defined(_MSC_VER) && defined(CPR_WIN32_SDK_MICROSOFT)
#if _MSC_VER >= 1600
#include <stdint.h>
#elif defined(CPR_STDINT_INCLUDE)
#include CPR_STDINT_INCLUDE
#else
typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned char  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif
#endif





typedef uint8_t boolean;









#ifndef _SSIZE_T_
#define _SSIZE_T_
typedef int ssize_t;
#endif




typedef int pid_t;





#ifndef MIN
#define MIN min
#endif

#ifndef MAX
#define MAX max
#endif







#endif 

