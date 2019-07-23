







































#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifdef XP_MAC
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#define NS_ASSERTION(c,m) 

#define PR_ASSERT         assert
#define PR_Malloc         malloc
#define PR_Free           free
#define PR_FREEIF(x)      do { if (x) free(x); } while(0)
#define PL_strfree        free
#define PL_strcmp         strcmp
#define PL_strdup         strdup
#define PL_strcpy         strcpy
#define PL_strlen         strlen

#define PR_Open(a,b,c)    fopen((a),(b)) // XXX Ignores mode
#define PR_Read(f,d,n)    fread((d),1,(n),(f))
#define PR_Write(f,s,n)   fwrite((s),1,(n),(f))
#define PR_Close          fclose
#define PR_Seek           fseek
#define PR_Delete         remove

#ifdef __cplusplus
#define PR_BEGIN_EXTERN_C       extern "C" {
#define PR_END_EXTERN_C         }
#else
#define PR_BEGIN_EXTERN_C   
#define PR_END_EXTERN_C  
#endif

#if defined(XP_MAC)
#define PR_EXTERN(__type)       extern __declspec(export) __type
#define PR_PUBLIC_API(__type)   __declspec(export) __type
#elif defined(XP_WIN)
#define PR_EXTERN(__type)       extern _declspec(dllexport) __type
#define PR_PUBLIC_API(__type)   _declspec(dllexport) __type
#elif defined(XP_OS2) && defined(__declspec)
#define PR_EXTERN(__type)       extern __declspec(dllexport) __type
#define PR_PUBLIC_API(__type)   __declspec(dllexport) __type
#else 
#define PR_EXTERN(__type)       extern __type
#define PR_PUBLIC_API(__type)   __type 
#endif

#define PR_CALLBACK
#define PR_STATIC_CALLBACK(__x) static __x

#define PRFileDesc          FILE
typedef long                PRInt32;
typedef PRInt32             PROffset32;
typedef unsigned long       PRUint32;
typedef short               PRInt16;
typedef unsigned short      PRUint16;
typedef char                PRBool;
typedef unsigned char       PRUint8;
typedef PRUint8             PRPackedBool;
typedef PRInt32             nsresult;

#define PR_TRUE             1
#define PR_FALSE            0

#define INVALID_SXP   -2
#define NON_SXP       -1
#define VALID_SXP     1
#define MATCH 0
#define NOMATCH 1
#define ABORTED -1

#define PR_RDONLY     "rb"
#define PR_SEEK_SET   SEEK_SET
#define PR_SEEK_END   SEEK_END

#define NS_WildCardValid(a)       NON_SXP
#define NS_WildCardMatch(a,b,c)   PR_FALSE

#define MOZ_COUNT_CTOR(x)
#define MOZ_COUNT_DTOR(x)
