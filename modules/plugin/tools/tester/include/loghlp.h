




































#ifndef __LOGHLP_H__
#define __LOGHLP_H__

#include "logger.h"

LPSTR FormatNPAPIError(int iError);
LPSTR FormatNPAPIReason(int iReason);
LPSTR FormatNPPVariable(NPPVariable var);
LPSTR FormatNPNVariable(NPNVariable var);
BOOL FormatPCHARArgument(LPSTR szBuf, int iLength, LogArgumentStruct * parg);
BOOL FormatBOOLArgument(LPSTR szBuf, int iLength, LogArgumentStruct * parg);
BOOL FormatPBOOLArgument(LPSTR szBuf, int iLength, LogArgumentStruct * parg);
LogItemStruct * makeLogItemStruct(NPAPI_Action action, 
                                  DWORD dwTimeEnter, DWORD dwTimeReturn, 
                                  DWORD dwRet,
                                  DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4,
                                  DWORD dw5, DWORD dw6, DWORD dw7, BOOL bShort = FALSE);
int formatLogItem(LogItemStruct * plis, LPSTR szOutput, LPSTR szEndOfItem, BOOL bDOSStyle = FALSE);

#endif 
