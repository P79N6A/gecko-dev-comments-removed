






































#ifndef _EXTERN_H_
#define _EXTERN_H_

#include "setup.h"


extern HINSTANCE        hInst;
extern HINSTANCE        hSetupRscInst;
extern HINSTANCE        hSDInst;
extern HINSTANCE        hXPIStubInst;

extern HBITMAP          hbmpBoxChecked;
extern HBITMAP          hbmpBoxCheckedDisabled;
extern HBITMAP          hbmpBoxUnChecked;

extern HANDLE           hAccelTable;

extern HWND             hDlgCurrent;
extern HWND             hDlgMessage;
extern HWND             hWndMain;

extern LPSTR            szEGlobalAlloc;
extern LPSTR            szEStringLoad;
extern LPSTR            szEDllLoad;
extern LPSTR            szEStringNull;
extern LPSTR            szEOutOfMemory;
extern LPSTR            szTempSetupPath;

extern LPSTR            szSetupDir;
extern LPSTR            szTempDir;
extern LPSTR            szOSTempDir;
extern LPSTR            szFileIniConfig;
extern LPSTR            szFileIniInstall;

extern LPSTR            szSiteSelectorDescription;

extern DWORD            dwWizardState;
extern DWORD            dwSetupType;

extern DWORD            dwTempSetupType;
extern DWORD            gdwUpgradeValue;
extern DWORD            gdwSiteSelectorStatus;

extern BOOL             bSDUserCanceled;
extern BOOL             bIdiArchivesExists;
extern BOOL             bCreateDestinationDir;
extern BOOL             bReboot;
extern BOOL             gbILUseTemp;
extern BOOL             gbPreviousUnfinishedDownload;
extern BOOL             gbIgnoreRunAppX;
extern BOOL             gbIgnoreProgramFolderX;
extern BOOL             gbRestrictedAccess;
extern BOOL             gbDownloadTriggered;
extern BOOL             gbAllowMultipleInstalls;
extern BOOL             gbForceInstall;
extern BOOL             gbForceInstallGre;
extern BOOL             gShowBannerImage;

extern setupGen         sgProduct;
extern diS              diSetup;
extern diW              diWelcome;
extern diQL             diQuickLaunch;
extern diL              diLicense;
extern diST             diSetupType;
extern diSC             diSelectComponents;
extern diSC             diSelectAdditionalComponents;
extern diWI             diWindowsIntegration;
extern diPF             diProgramFolder;
extern diDO             diAdditionalOptions;
extern diAS             diAdvancedSettings;
extern diSI             diStartInstall;
extern diD              diDownload;
extern diR              diReboot;
extern siSD             siSDObject;
extern siCF             siCFXpcomFile;
extern siC              *siComponents;
extern ssi              *ssiSiteSelector;
extern char             *SetupFileList[];
extern installGui       sgInstallGui;
extern sems             gErrorMessageStream;
extern sysinfo          gSystemInfo;
extern dsN              *gdsnComponentDSRequirement;

#endif 

