






































#ifndef MOZCE_ENVIRONMENT_H
#define MOZCE_ENVIRONMENT_H

#ifdef __cplusplus
extern "C" {
#endif


char* getenv(const char* inName);
int putenv(const char *a);
char SetEnvironmentVariableW(const unsigned short * name, const unsigned short * value );
char GetEnvironmentVariableW(const unsigned short * lpName, unsigned short* lpBuffer, unsigned long nSize);

unsigned int ExpandEnvironmentStringsW(const unsigned short* lpSrc,
				       unsigned short* lpDst,
				       unsigned int nSize);

unsigned short* mozce_GetEnvironmentCL();

#ifdef __cplusplus
};
#endif

#endif
