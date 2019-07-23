






































#ifndef _PARSER_H_
#define _PARSER_H_

sil         *InitSilNodes(char *szFileIni);
void        DeInitSilNodes(sil **silHead);
ULONG       ParseCommandLine(int argc, char *argv[]);
HRESULT     FileExists(PSZ szFile);
ULONG       Uninstall(sil* silFile);
void        ParseForFile(PSZ szString, PSZ szKey, PSZ szFile, ULONG ulShortFilenameBufSize);
void        ParseForCopyFile(PSZ szString, PSZ szKeyStr, PSZ szFile, ULONG ulShortFilenameBufSize);
void        ParseForWinRegInfo(PSZ szString, PSZ szKeyStr, PSZ szRootKey, ULONG ulRootKeyBufSize, PSZ szKey, ULONG ulKeyBufSize, PSZ szName, ULONG ulNameBufSize);
ULONG       GetLogFile(PSZ szTargetPath, PSZ szInFilename, PSZ szOutBuf, ULONG ulOutBufSize);
void        RemoveUninstaller(PSZ szUninstallFilename);
ULONG       DecrementSharedFileCounter(char *file);
BOOL        DeleteOrDelayUntilReboot(PSZ szFile);
BOOL        UnregisterServer(char *file);
int         GetSharedFileCount(char *file);
BOOL        DetermineUnRegisterServer(sil *silInstallLogHead, PSZ szFile);

#endif

