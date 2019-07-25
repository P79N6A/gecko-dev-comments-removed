






































#ifndef MOZCE_ENVIRONMENT_H
#define MOZCE_ENVIRONMENT_H

#ifdef __cplusplus
extern "C" {
#endif


char* getenv(const char* inName);
int putenv(const char *a);
char SetEnvironmentVariableW(const wchar_t * name, const wchar_t * value );
char GetEnvironmentVariableW(const wchar_t * lpName, wchar_t* lpBuffer, unsigned long nSize);

unsigned int ExpandEnvironmentStringsW(const wchar_t* lpSrc,
				       wchar_t* lpDst,
				       unsigned int nSize);

wchar_t* mozce_GetEnvironmentCL();

#ifdef __cplusplus
};
#endif

#endif
