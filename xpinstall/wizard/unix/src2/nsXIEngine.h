






































#ifndef _NS_XIENGINE_H_
#define _NS_XIENGINE_H_

#include "XIDefines.h"
#include "nsXInstaller.h"
#include "nsComponent.h"
#include "nsComponentList.h"
#include "nsInstallDlg.h"
#include "nsZipExtractor.h"

#include "xpistub.h"

#define STANDALONE 1
#include "zipfile.h"
#include <ctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>




typedef nsresult (*pfnXPI_Init) 
                 (const char *aProgramDir, const char *aLogName,
                  pfnXPIProgress progressCB);
typedef nsresult (*pfnXPI_Install) 
                 (const char *file, const char *args, long flags);
typedef void     (*pfnXPI_Exit)();

typedef struct _xpistub_t 
{
    const char      *name;
    void            *handle;
    pfnXPI_Init     fn_init;
    pfnXPI_Install  fn_install;
    pfnXPI_Exit     fn_exit;
} xpistub_t;

#define TYPE_UNDEF 0
#define TYPE_PROXY 1
#define TYPE_HTTP 2
#define TYPE_FTP 3

typedef struct _conn
{
  unsigned char type; 
  char *URL;      
  void *conn;     
} CONN;




class nsXIEngine
{
public:
    nsXIEngine();
    ~nsXIEngine();

    int     Download(int aCustom, nsComponentList *aComps);
    int     Extract(nsComponent *aXPIEngine);
    int     Install(int aCustom, nsComponentList *aComps, char *aDestination);
    int     DeleteXPIs(int aCustom, nsComponentList *aComps);

    static void ProgressCallback(const char* aMsg, PRInt32 aVal, PRInt32 aMax);
    static int  ExistAllXPIs(int aCustom, nsComponentList *aComps);

    enum
    {
        OK          = 0,
        E_PARAM     = -1201,
        E_MEM       = -1202,
        E_OPEN_MKR  = -1203,
        E_WRITE_MKR = -1204,
        E_READ_MKR  = -1205,
        E_FIND_COMP = -1206,
        E_STAT      = -1207
    };

private:
    int     MakeUniqueTmpDir();
    int     LoadXPIStub(xpistub_t *aStub, char *aDestionation);
    int     InstallXPI(nsComponent *aComp, xpistub_t *aStub);
    int     UnloadXPIStub(xpistub_t *aStub);
    int     GetFileSize(char *aPath);
    int     SetDLMarker(char *aCompName);
    int     GetDLMarkedComp(nsComponentList *aComps, nsComponent **aOutComp);
    int     DelDLMarker();
    int     TotalToDownload(int aCustom, nsComponentList *aComps);
    PRBool  CRCCheckDownloadedArchives(char *dlPath, short dlPathLen, 
              nsComponentList *aComps, int aCustom);
    PRBool  IsArchiveFile(char *path);
    PRBool  CheckConn( char *URL, int type, CONN *myConn, PRBool force );

    static int  VerifyArchive(char *szArchive);
    
    char    *mTmp;
    char    *mOriginalDir;
};

#endif 
