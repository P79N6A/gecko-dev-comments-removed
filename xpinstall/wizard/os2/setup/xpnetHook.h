




































#ifndef _XPNETHOOK_H_
#define _XPNETHOOK_H_

int WGet(char *szUrl,
         char *szFile,
         char *szProxyServer,
         char *szProxyPort,
         char *szProxyUser,
         char *szProxyPasswd);
int DownloadFiles(char *szInputIniFile,
                  char *szDownloadDir,
                  char *szProxyServer,
                  char *szProxyPort,
                  char *szProxyUser,
                  char *szProxyPasswd,
                  BOOL bShowRetryMsg,
                  BOOL bIgnoreNetworkError,
                  char *szFailedFile,
                  DWORD dwFailedFileSize);

#endif 

