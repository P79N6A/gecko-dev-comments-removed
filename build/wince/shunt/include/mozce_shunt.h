





































#ifndef MOZCE_SHUNT_H
#define MOZCE_SHUNT_H

#ifdef MOZCE_SHUNT_EXPORTS
#define _CRTIMP __declspec(dllexport)
#endif

#ifdef MOZ_MEMORY
#ifdef __cplusplus


#define _NEW_
void * operator new(size_t _Size);
void operator delete(void * ptr);
void *operator new[](size_t size);
void operator delete[](void *ptr);

extern "C" {
#endif

extern void* moz_malloc(size_t);
extern void* moz_valloc(size_t);
extern void* moz_calloc(size_t, size_t);
extern void* moz_realloc(void*, unsigned int);
extern void moz_free(void*);
  
void* __cdecl  malloc(size_t);
void* __cdecl  valloc(size_t);
void* __cdecl  calloc(size_t, size_t);
void* __cdecl  realloc(void*, unsigned int);
void __cdecl  free(void*);

 
char*
mozce_strdup(const char*);

MOZCE_SHUNT_API unsigned short* 
mozce_wcsdup(const unsigned short* );

MOZCE_SHUNT_API char*
mozce_strndup(const char *, unsigned int);

MOZCE_SHUNT_API unsigned short* 
mozce_wcsndup(const unsigned short*, unsigned int);
  
#ifdef __cplusplus
}   
#endif


#undef _strdup
#undef strdup
#undef _strndup
#undef strndup
#undef _wcsdup
#undef wcsdup
#undef _wcsndup
#undef wcsndup



#define _strdup mozce_strdup
#define _strndup mozce_strndup

#define _wcsdup mozce_wcsdup
#define _wcsndup mozce_wcsndup

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


extern MOZCE_SHUNT_IMPORT_API int errno;
MOZCE_SHUNT_IMPORT_API char* strerror(int);


MOZCE_SHUNT_API void abort(void);
  

MOZCE_SHUNT_API char* getenv(const char* inName);
MOZCE_SHUNT_API int putenv(const char *a);
MOZCE_SHUNT_API char SetEnvironmentVariableW(const unsigned short * name, const unsigned short * value );
MOZCE_SHUNT_API char GetEnvironmentVariableW(const unsigned short * lpName, unsigned short* lpBuffer, unsigned long nSize);
  
MOZCE_SHUNT_API unsigned int ExpandEnvironmentStringsW(const unsigned short* lpSrc,
                                                       unsigned short* lpDst,
                                                       unsigned int nSize);


MOZCE_SHUNT_API unsigned short * _wgetcwd(unsigned short* dir, unsigned long size);
MOZCE_SHUNT_API unsigned short *_wfullpath( unsigned short *absPath, const unsigned short *relPath, unsigned long maxLength );
MOZCE_SHUNT_API int _unlink(const char *filename );
  












  

  








MOZCE_SHUNT_API unsigned short* mozce_GetEnvironmentCL();

   
#define M_SQRT1_2  0.707106781186547524401

#ifdef __cplusplus
};
#endif

#endif
