





































#ifndef MOZCE_SHUNT_H
#define MOZCE_SHUNT_H

#ifdef MOZCE_SHUNT_EXPORTS
#define MOZCE_SHUNT_API __declspec(dllexport)
#else
#define MOZCE_SHUNT_API __declspec(dllimport)
#endif
  
#define strdup  _strdup
#define strcmpi _stricmp
#define stricmp _stricmp
#define wgetcwd _wgetcwd

#ifdef __cplusplus
extern "C" {
#endif

MOZCE_SHUNT_API void abort(void);
  

MOZCE_SHUNT_API char* getenv(const char* inName);
MOZCE_SHUNT_API int putenv(const char *a);
MOZCE_SHUNT_API char SetEnvironmentVariableW(const unsigned short * name, const unsigned short * value );
MOZCE_SHUNT_API char GetEnvironmentVariableW(const unsigned short * lpName, unsigned short* lpBuffer, unsigned long nSize);
  

MOZCE_SHUNT_API unsigned short * _wgetcwd(unsigned short* dir, unsigned long size);
MOZCE_SHUNT_API unsigned short *_wfullpath( unsigned short *absPath, const unsigned short *relPath, unsigned long maxLength );
MOZCE_SHUNT_API int _unlink(const char *filename );
  












  

  








#ifdef __cplusplus
};
#endif

#endif
