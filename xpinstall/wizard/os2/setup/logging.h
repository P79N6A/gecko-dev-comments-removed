




































#ifndef _LOGGING_H_
#define _LOGGING_H_

int               AppendToGlobalMessageStream(char *szInfo);
void              LogISTime(int iType);
void              LogISProductInfo(void);
void              LogISDestinationPath(void);
void              LogISSetupType(void);
void              LogISComponentsSelected(void);
void              LogISComponentsToDownload(void);
void              LogISComponentsFailedCRC(char *szList, int iWhen);
void              LogISDownloadStatus(char *szStatus, char *szFailedFile);
void              LogISDownloadProtocol(DWORD dwProtocolType);
void              LogISXPInstall(int iWhen);
void              LogISXPInstallComponent(char *szComponentName);
void              LogISXPInstallComponentResult(DWORD dwErrorNumber);
void              LogISLaunchApps(int iWhen);
void              LogISLaunchAppsComponent(char *szComponentName);
void              LogISLaunchAppsComponentUncompress(char *szComponentName,
                                                     DWORD dwErr);
void              LogISProcessXpcomFile(int iStatus, int iResult);
void              LogISDiskSpace(dsN *dsnComponentDSRequirement);
void              LogISTurboMode(BOOL bTurboMode);
void              LogMSProductInfo(void);
void              LogMSDownloadFileStatus(void);
void              LogMSDownloadStatus(int iDownloadStatus);
void              LogMSDownloadProtocol(DWORD dwProtocolType);
void              LogMSXPInstallStatus(char *szFile, int iErr);
void              LogMSTurboMode(BOOL bTurboMode);

#endif 

