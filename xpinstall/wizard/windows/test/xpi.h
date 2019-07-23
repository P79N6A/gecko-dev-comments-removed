






































#ifndef _XPI_H_
#define _XPI_H_

HRESULT         InitializeXPIStub(void);
HRESULT         DeInitializeXPIStub(void);
HRESULT         SmartUpdateJars(LPSTR szAppName, LPSTR szAppPath, LPSTR listArchive[]);
void            cbXPIStart(const char *, const char *UIName);
void            cbXPIProgress(const char* msg, PRInt32 val, PRInt32 max);
void            cbXPIFinal(const char *, PRInt32 finalStatus);
void            InitProgressDlg(void);
void            DeInitProgressDlg(void);
int             GetTotalArchivesToInstall(LPSTR listArchive[]);
void            ProcessWindowsMessages();

#endif 

