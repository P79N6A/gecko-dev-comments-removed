




































#ifndef MOZCE_SHUNT_H
#define MOZCE_SHUNT_H

#include "environment.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifdef __cplusplus
}   
#endif

#ifdef MOZ_MEMORY

#ifdef __cplusplus



#define _NEW_
#define _INC_NEW

#ifndef __NOTHROW_T_DEFINED
#define __NOTHROW_T_DEFINED
namespace std {
  struct nothrow_t {};
  extern const nothrow_t nothrow;
  struct bad_alloc {};
};
#endif


#include "jemalloc.h"




inline void *operator new(size_t size, const std::nothrow_t&) throw() {
  return malloc(size);
}
inline void *operator new[](size_t size, const std::nothrow_t&) throw() {
  return malloc(size);
}
inline void operator delete(void* ptr, const std::nothrow_t&) throw() {
  free(ptr);
}
inline void operator delete[](void* ptr, const std::nothrow_t&) throw() {
  free(ptr);
}



inline void *operator new(size_t, void *p) {
  return p;
}
inline void *operator new[](size_t, void *p) {
  return p;
}





#if defined(_MOZILLA_CONFIG_H_) && !defined(XPCOM_GLUE) && !defined(NS_NO_XPCOM) && !defined(MOZ_NO_MOZALLOC)
#  include "mozilla/mozalloc.h"
#else

inline void* operator new(size_t size) throw() {
  return malloc(size);
}
inline void* operator new[](size_t size) throw() {
  return malloc(size);
}
inline void operator delete(void* ptr) throw() {
  free(ptr);
}
inline void operator delete[](void* ptr) throw() {
  free(ptr);
}

#endif  


extern "C" {
#endif

#undef _strdup
#undef strdup
#undef _strndup
#undef strndup
#undef _wcsdup
#undef wcsdup
#undef _wcsndup
#undef wcsndup




char * __cdecl
_strdup(const char*);

wchar_t * __cdecl
_wcsdup(const wchar_t *);

char * __cdecl
_strndup(const char *, unsigned int);

wchar_t * __cdecl
_wcsndup(const wchar_t *, unsigned int);
  
#ifdef __cplusplus
}   
#endif

#endif

#define strdup  _strdup
#define strndup _strndup
#define wcsdup _wcsdup
#define wcsndup _wcsndup


#define strcmpi _stricmp
#define stricmp _stricmp
#define wgetcwd _wgetcwd
#define vsnprintf _vsnprintf

#define SHGetSpecialFolderPathW SHGetSpecialFolderPath
#define SHGetPathFromIDListW    SHGetPathFromIDList
#define FONTENUMPROCW           FONTENUMPROC

#ifdef __cplusplus
extern "C" {
#endif


extern int errno;
char* strerror(int);


void abort(void);
  

char* getenv(const char* inName);
int putenv(const char *a);
char SetEnvironmentVariableW(const unsigned short * name, const unsigned short * value );
char GetEnvironmentVariableW(const unsigned short * lpName, unsigned short* lpBuffer, unsigned long nSize);
  
unsigned int ExpandEnvironmentStringsW(const unsigned short* lpSrc,
				       unsigned short* lpDst,
				       unsigned int nSize);


unsigned short * _wgetcwd(unsigned short* dir, unsigned long size);
unsigned short *_wfullpath( unsigned short *absPath, const unsigned short *relPath, unsigned long maxLength );
int _unlink(const char *filename );
int _wchdir(const unsigned short* path);
  













struct tm;

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

struct tm* gmtime_r(const time_t* inTimeT, struct tm* outRetval);
struct tm* localtime_r(const time_t* inTimeT, struct tm* outRetval);


  








unsigned short* mozce_GetEnvironmentCL();

   
#define M_SQRT1_2  0.707106781186547524401

#ifdef __cplusplus
};
#endif

#endif
