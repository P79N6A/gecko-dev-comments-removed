






































#include <os2.h>

#ifndef _XPI_H_
#define _XPI_H_

APIRET         InitializeXPIStub(void);
APIRET         DeInitializeXPIStub(void);
APIRET         SmartUpdateJars(PSZ szAppName, PSZ szAppPath, PSZ listArchive[]);
void            cbXPIStart(const char *, const char *UIName);
void            cbXPIProgress(const char* msg, PRInt32 val, PRInt32 max);
void            cbXPIFinal(const char *, PRInt32 finalStatus);
void            InitProgressDlg(void);
void            DeInitProgressDlg(void);
int             GetTotalArchivesToInstall(PSZ listArchive[]);
void            ProcessWindowsMessages();

#endif 

