




































#ifndef WINVER
#define WINVER 0x0600
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0700
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef CITYHASH_EXPORTS
#define CITYHASH_API __declspec(dllexport)
#else
#define CITYHASH_API __declspec(dllimport)
#endif

#ifndef ssize_t
typedef int ssize_t;
#endif
