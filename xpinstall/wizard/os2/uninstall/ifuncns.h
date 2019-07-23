






































#ifndef _IFUNCNS_H_
#define _IFUNCNS_H_

#include "uninstall.h"

HRESULT     FileUncompress(PSZ szFrom, PSZ szTo);
HRESULT     FileMove(PSZ szFrom, PSZ szTo);
HRESULT     FileCopy(PSZ szFrom, PSZ szTo, BOOL bFailIfExists);
HRESULT     FileDelete(PSZ szDestination);
HRESULT     DirectoryRemove(PSZ szDestination, BOOL bRemoveSubdirs);
HRESULT     CreateDirectoriesAll(char* szPath);


BOOL        SearchForUninstallKeys(char *szStringToMatch);

#endif
