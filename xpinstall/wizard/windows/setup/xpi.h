






































#ifndef _XPI_H_
#define _XPI_H_

HRESULT         InitializeXPIStub(char *xpinstallPath);
HRESULT         DeInitializeXPIStub(void);
HRESULT         SmartUpdateJars(void);
void            cbXPIStart(const char *, const char *UIName);
void            cbXPIProgress(const char* msg, PRInt32 val, PRInt32 max);
void            cbXPIFinal(const char *, PRInt32 finalStatus);
void            InitProgressDlg(void);
void            DeInitProgressDlg(void);
void            GetTotalArchivesToInstall(void);
char            *GetErrorString(DWORD dwError, char *szErrorString, DWORD dwErrorStringSize);

#endif 

