






































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

#include "windows.h"
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

void            PrintError(LPSTR szMsg, DWORD dwErrorCodeSH, int iExitCode);
void            RemoveQuotes(LPSTR lpszSrc, LPSTR lpszDest, int dwDestSize);
void            RemoveBackSlash(LPSTR szInput);
void            AppendBackSlash(LPSTR szInput, DWORD dwInputSize);
void            ParsePath(LPSTR szInput, LPSTR szOutput, DWORD dwOutputSize, DWORD dwType);
long            FileExists(LPSTR szFile);
int             GetArgC(LPSTR lpszCommandLine);
LPSTR           GetArgV(LPSTR lpszCommandLine, int iIndex, LPSTR lpszDest, int iDestSize);

#endif

