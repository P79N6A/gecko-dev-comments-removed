



































#ifndef ubase_h__
#define ubase_h__

#ifdef _WIN32
#ifndef NS_WIN32 
#define NS_WIN32 1
#endif
#endif

#if defined(__unix)
#ifndef NS_UNIX 
#define NS_UNIX 1
#endif
#endif

#include "prtypes.h"

#define PRIVATE 
#define MODULE_PRIVATE

#endif
