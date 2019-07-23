






































#ifndef _EXTRA_H_
#define _EXTRA_H_

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

typedef long HRESULT;

typedef struct diskSpaceNode dsN;
struct diskSpaceNode
{
  unsigned long long       ullSpaceRequired;
  PSZ             szPath;
  dsN             *Next;
  dsN             *Prev;
};

typedef struct structVer
{
  unsigned long long ullMajor;
  unsigned long long ullMinor;
  unsigned long long ullRelease;
  unsigned long long ullBuild;
} verBlock;

BOOL              InitApplication(HMODULE hInstance);
BOOL              InitInstance(HMODULE hInstance, ULONG dwCmdShow);
void              PrintError(PSZ   szMsg, ULONG dwErrorCodeSH);
void              FreeMemory(void **vPointer);
void              *NS_GlobalAlloc(ULONG dwMaxBuf);
HRESULT           Initialize(HMODULE hInstance, PSZ szAppName);
HRESULT           NS_LoadStringAlloc(HMODULE hInstance, ULONG dwID, PSZ   *szStringBuf, ULONG dwStringBuf);
HRESULT           NS_LoadString(HMODULE hInstance, ULONG dwID, PSZ   szStringBuf, ULONG dwStringBuf);
HRESULT           WinSpawn(PSZ   szClientName, PSZ   szParameters, PSZ   szCurrentDir, BOOL bWait);
HRESULT           ParseConfigIni(PSZ   lpszCmdLine);
HRESULT           DecryptString(PSZ   szOutputStr, PSZ   szInputStr);
HRESULT           DecryptVariable(PSZ   szVariable, ULONG dwVariableSize);



HRESULT           InitUninstallGeneral(void);
HRESULT           InitDlgUninstall(diU *diDialog);
sil               *CreateSilNode();
void              SilNodeInsert(sil *silHead, sil *silTemp);
void              SilNodeDelete(sil *silTemp);
void              DeInitialize(void);
void              DeInitDlgUninstall(diU *diDialog);
void              DetermineOSVersion(void);
void              DeInitILomponents(void);
void              DeInitUninstallGeneral(void);
HRESULT           ParseUninstallIni(int argc, char *argv[]);
void              ParsePath(PSZ   szInput, PSZ   szOutput, ULONG dwLength, ULONG dwType);
void              RemoveBackSlash(PSZ   szInput);
void              AppendBackSlash(PSZ   szInput, ULONG dwInputSize);
void              RemoveSlash(PSZ   szInput);
void              AppendSlash(PSZ   szInput, ULONG dwInputSize);
HRESULT           FileExists(PSZ   szFile);
BOOL              isFAT(char* szPath);
HRESULT           CheckInstances();
void              RemoveQuotes(PSZ   lpszSrc, PSZ   lpszDest, int iDestSize);
PSZ               GetFirstNonSpace(PSZ   lpszString);
ULONG             ParseCommandLine(int argc, char *argv[]);
void              SetUninstallRunMode(PSZ   szMode);
void              Delay(ULONG dwSeconds);
HRESULT           GetAppPath();
ULONG             CleanupAppList();

HRESULT           GetUninstallLogPath();

HWND              FindWindow(PCSZ pszAtomString);

#endif

