









































#ifndef _SECPORT_H_
#define _SECPORT_H_

#include "utilrename.h"
#include "prlink.h"





#ifdef _WINDOWS
# ifndef XP_WIN
# define XP_WIN
# endif
#if defined(_WIN32) || defined(WIN32)
# ifndef XP_WIN32
# define XP_WIN32
# endif
#endif
#endif

#ifdef __BEOS__
# ifndef XP_BEOS
# define XP_BEOS
# endif
#endif

#ifdef unix
# ifndef XP_UNIX
# define XP_UNIX
# endif
#endif

#if defined(_WIN32_WCE)
#include <windef.h>
#include <types.h>
#else
#include <sys/types.h>
#endif

#include <ctype.h>
#include <string.h>
#if defined(_WIN32_WCE)
#include <stdlib.h>	
#else
#include <stddef.h>
#endif
#include <stdlib.h>
#include "prtypes.h"
#include "prlog.h"	
#include "plarena.h"
#include "plstr.h"




#ifndef SEC_BEGIN_PROTOS
#include "seccomon.h"
#endif

SEC_BEGIN_PROTOS

extern void *PORT_Alloc(size_t len);
extern void *PORT_Realloc(void *old, size_t len);
extern void *PORT_AllocBlock(size_t len);
extern void *PORT_ReallocBlock(void *old, size_t len);
extern void PORT_FreeBlock(void *ptr);
extern void *PORT_ZAlloc(size_t len);
extern void PORT_Free(void *ptr);
extern void PORT_ZFree(void *ptr, size_t len);
extern char *PORT_Strdup(const char *s);
extern time_t PORT_Time(void);
extern void PORT_SetError(int value);
extern int PORT_GetError(void);

extern PLArenaPool *PORT_NewArena(unsigned long chunksize);
extern void *PORT_ArenaAlloc(PLArenaPool *arena, size_t size);
extern void *PORT_ArenaZAlloc(PLArenaPool *arena, size_t size);
extern void PORT_FreeArena(PLArenaPool *arena, PRBool zero);
extern void *PORT_ArenaGrow(PLArenaPool *arena, void *ptr,
			    size_t oldsize, size_t newsize);
extern void *PORT_ArenaMark(PLArenaPool *arena);
extern void PORT_ArenaRelease(PLArenaPool *arena, void *mark);
extern void PORT_ArenaZRelease(PLArenaPool *arena, void *mark);
extern void PORT_ArenaUnmark(PLArenaPool *arena, void *mark);
extern char *PORT_ArenaStrdup(PLArenaPool *arena, const char *str);

SEC_END_PROTOS

#define PORT_Assert PR_ASSERT
#define PORT_ZNew(type) (type*)PORT_ZAlloc(sizeof(type))
#define PORT_New(type) (type*)PORT_Alloc(sizeof(type))
#define PORT_ArenaNew(poolp, type)	\
		(type*) PORT_ArenaAlloc(poolp, sizeof(type))
#define PORT_ArenaZNew(poolp, type)	\
		(type*) PORT_ArenaZAlloc(poolp, sizeof(type))
#define PORT_NewArray(type, num)	\
		(type*) PORT_Alloc (sizeof(type)*(num))
#define PORT_ZNewArray(type, num)	\
		(type*) PORT_ZAlloc (sizeof(type)*(num))
#define PORT_ArenaNewArray(poolp, type, num)	\
		(type*) PORT_ArenaAlloc (poolp, sizeof(type)*(num))
#define PORT_ArenaZNewArray(poolp, type, num)	\
		(type*) PORT_ArenaZAlloc (poolp, sizeof(type)*(num))



#define PORT_Atoi(buff)	(int)strtol(buff, NULL, 10)







#define PORT_ErrorToString(err) PR_ErrorToString((err), PR_LANGUAGE_I_DEFAULT)

#define PORT_ErrorToName PR_ErrorToName

#define PORT_Memcmp 	memcmp
#define PORT_Memcpy 	memcpy
#ifndef SUNOS4
#define PORT_Memmove 	memmove
#else 
#define PORT_Memmove(s,ct,n)    bcopy ((ct), (s), (n))
#endif
#define PORT_Memset 	memset

#define PORT_Strcasecmp PL_strcasecmp
#define PORT_Strcat 	strcat
#define PORT_Strchr 	strchr
#define PORT_Strrchr    strrchr
#define PORT_Strcmp 	strcmp
#define PORT_Strcpy 	strcpy
#define PORT_Strlen(s) 	strlen(s)
#define PORT_Strncasecmp PL_strncasecmp
#define PORT_Strncat 	strncat
#define PORT_Strncmp 	strncmp
#define PORT_Strncpy 	strncpy
#define PORT_Strpbrk    strpbrk
#define PORT_Strstr 	strstr
#define PORT_Strtok 	strtok

#define PORT_Tolower 	tolower

typedef PRBool (PR_CALLBACK * PORTCharConversionWSwapFunc) (PRBool toUnicode,
			unsigned char *inBuf, unsigned int inBufLen,
			unsigned char *outBuf, unsigned int maxOutBufLen,
			unsigned int *outBufLen, PRBool swapBytes);

typedef PRBool (PR_CALLBACK * PORTCharConversionFunc) (PRBool toUnicode,
			unsigned char *inBuf, unsigned int inBufLen,
			unsigned char *outBuf, unsigned int maxOutBufLen,
			unsigned int *outBufLen);

SEC_BEGIN_PROTOS

void PORT_SetUCS4_UTF8ConversionFunction(PORTCharConversionFunc convFunc);
void PORT_SetUCS2_ASCIIConversionFunction(PORTCharConversionWSwapFunc convFunc);
PRBool PORT_UCS4_UTF8Conversion(PRBool toUnicode, unsigned char *inBuf,
			unsigned int inBufLen, unsigned char *outBuf,
			unsigned int maxOutBufLen, unsigned int *outBufLen);
PRBool PORT_UCS2_ASCIIConversion(PRBool toUnicode, unsigned char *inBuf,
			unsigned int inBufLen, unsigned char *outBuf,
			unsigned int maxOutBufLen, unsigned int *outBufLen,
			PRBool swapBytes);
void PORT_SetUCS2_UTF8ConversionFunction(PORTCharConversionFunc convFunc);
PRBool PORT_UCS2_UTF8Conversion(PRBool toUnicode, unsigned char *inBuf,
			unsigned int inBufLen, unsigned char *outBuf,
			unsigned int maxOutBufLen, unsigned int *outBufLen);


PRBool PORT_ISO88591_UTF8Conversion(const unsigned char *inBuf,
			unsigned int inBufLen, unsigned char *outBuf,
			unsigned int maxOutBufLen, unsigned int *outBufLen);

extern PRBool
sec_port_ucs4_utf8_conversion_function
(
  PRBool toUnicode,
  unsigned char *inBuf,
  unsigned int inBufLen,
  unsigned char *outBuf,
  unsigned int maxOutBufLen,
  unsigned int *outBufLen
);

extern PRBool
sec_port_ucs2_utf8_conversion_function
(
  PRBool toUnicode,
  unsigned char *inBuf,
  unsigned int inBufLen,
  unsigned char *outBuf,
  unsigned int maxOutBufLen,
  unsigned int *outBufLen
);


extern PRBool
sec_port_iso88591_utf8_conversion_function
(
  const unsigned char *inBuf,
  unsigned int inBufLen,
  unsigned char *outBuf,
  unsigned int maxOutBufLen,
  unsigned int *outBufLen
);

extern int NSS_PutEnv(const char * envVarName, const char * envValue);

extern int NSS_SecureMemcmp(const void *a, const void *b, size_t n);































PRLibrary *
PORT_LoadLibraryFromOrigin(const char* existingShLibName,
                 PRFuncPtr staticShLibFunc,
                 const char *newShLibName);

SEC_END_PROTOS

#endif 
