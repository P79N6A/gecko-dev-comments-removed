







































#ifndef _TESTXPI_H_
#define _TESTXPI_H_

#ifdef __cplusplus
#define PR_BEGIN_EXTERN_C       extern "C" {
#define PR_END_EXTERN_C         }
#else
#define PR_BEGIN_EXTERN_C
#define PR_END_EXTERN_C
#endif

typedef unsigned  int PRUint32;
typedef           int PRInt32;

#define PR_EXTERN(type) type

#include <os2.h>
#include "xpi.h"
#include "..\\setup\\xpistub.h"
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_BUF                         4096

#define ERROR_CODE_HIDE                 0
#define ERROR_CODE_SHOW                 1


#define PP_FILENAME_ONLY                1
#define PP_PATH_ONLY                    2
#define PP_ROOT_ONLY                    3

#define TEST_OK                         0

void            PrintError(PSZ szMsg, ULONG dwErrorCodeSH, int iExitCode);
void            RemoveQuotes(PSZ pszSrc, PSZ pszDest, int DestSize);
void            RemoveBackSlash(PSZ szInput);
void            AppendBackSlash(PSZ szInput, ULONG InputSize);
void            ParsePath(PSZ szInput, PSZ szOutput, ULONG OutputSize, ULONG Type);
long            FileExists(PSZ szFile);
int             GetArgC(PSZ pszCommandLine);
PSZ           GetArgV(PSZ pszCommandLine, int iIndex, PSZ pszDest, int iDestSize);

#endif

