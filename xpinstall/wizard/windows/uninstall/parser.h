






































#ifndef _PARSER_H_
#define _PARSER_H_

sil         *InitSilNodes(char *szFileIni);
void        DeInitSilNodes(sil **silHead);
HRESULT     FileExists(LPSTR szFile);
DWORD       Uninstall(sil* silFile);
void        ParseForFile(LPSTR szString, LPSTR szKey, LPSTR szFile, DWORD dwShortFilenameBufSize);
void        ParseForCopyFile(LPSTR szString, LPSTR szKeyStr, LPSTR szFile, DWORD dwShortFilenameBufSize);
HRESULT     ParseForWinRegInfo(LPSTR szString, LPSTR szKeyStr, LPSTR szRootKey, DWORD dwRootKeyBufSize, LPSTR szKey, DWORD dwKeyBufSize, LPSTR szName, DWORD dwNameBufSize);
void        ParseForUninstallCommand(LPSTR szString, LPSTR szKeyStr, LPSTR szFile, DWORD dwFileBufSize, LPSTR szParam, DWORD dwParamBufSize);
void        DeleteWinRegKey(HKEY hkRootKey, LPSTR szKey, BOOL bAbsoluteDelete);
DWORD       GetLogFile(LPSTR szTargetPath, LPSTR szInFilename, LPSTR szOutBuf, DWORD dwOutBufSize);
void        RemoveUninstaller(LPSTR szUninstallFilename);
DWORD       DecrementSharedFileCounter(char *file);
BOOL        DeleteOrDelayUntilReboot(LPSTR szFile);
BOOL        UnregisterServer(char *file);
int         GetSharedFileCount(char *file);
BOOL        DetermineUnRegisterServer(sil *silInstallLogHead, LPSTR szFile);

#endif

