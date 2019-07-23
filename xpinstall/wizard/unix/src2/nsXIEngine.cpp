







































#include "nsFTPConn.h"
#include "nsHTTPConn.h"
#include "nsXIEngine.h"

#include <errno.h>
#include <stdlib.h>

#define CORE_LIB_COUNT 13

const char kHTTPProto[] = "http://";
const char kFTPProto[] = "ftp://";
const char kDLMarkerPath[] = "./xpi/.current_download";

nsXIEngine::nsXIEngine() :
    mTmp(NULL),
    mOriginalDir(NULL)
{
}

#define RM_PREFIX "rm -rf "

nsXIEngine::~nsXIEngine()   
{
    DUMP("~nsXIEngine");

    
    chdir(mOriginalDir);

    if ( mTmp != (char *) NULL ) {

      

      char *buf;
      buf = (char *) malloc( strlen(RM_PREFIX) + strlen( mTmp ) + 1 );
      if ( buf != (char *) NULL ) {
        strcpy( buf, RM_PREFIX );
        strcat( buf, mTmp );
        system( buf ); 
        XI_IF_FREE(mTmp);
        free( buf );
      }
    }
    XI_IF_FREE(mOriginalDir);
}

int     
EventPumpCB(void)
{
    return 0;
}

#define MAXCRC 4

int     
nsXIEngine::Download(int aCustom, nsComponentList *aComps)
{
    DUMP("Download");

    if (!aComps)
        return E_PARAM;

    int err = OK;
    nsComponent *currComp = NULL, *markedComp = NULL;
    char *currURL = NULL;
    char *currHost = NULL;
    char *currPath = NULL;
    char localPath[MAXPATHLEN];
    char *srvPath = NULL;
    char *proxyURL = NULL;
    char *qualURL = NULL;
    int i, crcPass, bDone;
    int currPort;
    struct stat stbuf;
    int resPos = 0;
    int fileSize = 0;
    int currCompNum = 1;
    int numToDL = 0; 
    CONN myConn;
    
    err = GetDLMarkedComp(aComps, &markedComp);
    if (err == OK && markedComp)
    {
        sprintf(localPath, "%s/%s", XPI_DIR, markedComp->GetArchive());
        markedComp->SetResumePos(GetFileSize(localPath));
    }

    
    if (0 != stat(XPI_DIR, &stbuf))
    {
        if (0 != mkdir(XPI_DIR, 0755))
            return E_MKDIR_FAIL;
    }

    numToDL = TotalToDownload(aCustom, aComps);

    currComp = aComps->GetHead();

    myConn.URL = (char *) NULL;
    myConn.type = TYPE_UNDEF;

    crcPass = 0;
    bDone = 0;
    while ( bDone == 0 && crcPass < MAXCRC ) {
      while (currComp)
      {
        if ( (aCustom == TRUE && currComp->IsSelected()) || (aCustom == FALSE) )
        {
            
            if (currComp->IsDownloaded())
            {
                currComp = aComps->GetNext();
                continue;
            }

            SetDLMarker(currComp->GetArchive());

            for (i = 0; i < MAX_URLS; i++)
            {
                currURL = currComp->GetURL(i);
                if (!currURL) break;
                
                if (gCtx->opt->mMode != nsXIOptions::MODE_SILENT)
                    nsInstallDlg::SetDownloadComp(currComp, i, 
                        currCompNum, numToDL);

                
                resPos = currComp->GetResumePos();

                
                if (gCtx->opt->mProxyHost && gCtx->opt->mProxyPort)
                {
                    
                    proxyURL = (char *) malloc(strlen(kHTTPProto) + 
                                        strlen(gCtx->opt->mProxyHost) + 1 +
                                        strlen(gCtx->opt->mProxyPort) + 1);
                    if (!proxyURL)
                    {
                        err = E_MEM;
                        break;
                    }

                    sprintf(proxyURL, "%s%s:%s", kHTTPProto,
                            gCtx->opt->mProxyHost, gCtx->opt->mProxyPort);

                    nsHTTPConn *conn = new nsHTTPConn(proxyURL, EventPumpCB);
                    if (!conn)
                    {
                        err = E_MEM;
                        break;
                    }

                    
                    qualURL = (char *) malloc(strlen(currURL) + 
                                       strlen(currComp->GetArchive()) + 1);
                    if (!qualURL)
                    {
                        err = E_MEM;
                        break;
                    }
                    sprintf(qualURL, "%s%s", currURL, currComp->GetArchive());

                    if (*gCtx->opt->mProxyUser || *gCtx->opt->mProxyPswd)
                    {
                      conn->SetProxyInfo(qualURL, gCtx->opt->mProxyUser,
                                                  gCtx->opt->mProxyPswd);
                    }
                    else
                    {
                      conn->SetProxyInfo(qualURL, NULL, NULL);
                    }

                    err = conn->Open();
                    if (err == nsHTTPConn::OK)
                    {
                        sprintf(localPath, "%s/%s", XPI_DIR,
                            currComp->GetArchive());
                        if (gCtx->opt->mMode == nsXIOptions::MODE_SILENT)
                          err = conn->Get(NULL, localPath, resPos);
                        else
                          err = conn->Get(nsInstallDlg::DownloadCB, localPath,
                                          resPos);
                        conn->Close();
                    }
                    
                    XI_IF_FREE(proxyURL);
                    XI_IF_FREE(qualURL);
                    XI_IF_DELETE(conn);
                }
            
                
                else if (strncmp(currURL, kHTTPProto, strlen(kHTTPProto)) == 0)
                {
                    
                    qualURL = (char *) malloc(strlen(currURL) + 
                                       strlen(currComp->GetArchive()) + 1);
                    if (!qualURL)
                    {
                        err = E_MEM;
                        break;
                    }
                    sprintf(qualURL, "%s%s", currURL, currComp->GetArchive());

                    nsHTTPConn *conn = new nsHTTPConn(qualURL, EventPumpCB);
                    if (!conn)
                    {
                        err = E_MEM;
                        break;
                    }
    
                    err = conn->Open();
                    if (err == nsHTTPConn::OK)
                    {
                        sprintf(localPath, "%s/%s", XPI_DIR,
                            currComp->GetArchive());
                        if (gCtx->opt->mMode == nsXIOptions::MODE_SILENT)
                          err = conn->Get(NULL, localPath, resPos);
                        else
                          err = conn->Get(nsInstallDlg::DownloadCB, localPath,
                                          resPos);
                        conn->Close();
                    }

                    XI_IF_FREE(qualURL);
                    XI_IF_DELETE(conn);
                }

                
                else if (strncmp(currURL, kFTPProto, strlen(kFTPProto)) == 0)
                {
                    PRBool isNewConn;

                    err = nsHTTPConn::ParseURL(kFTPProto, currURL, &currHost, 
                            &currPort, &currPath);
                    if (err != nsHTTPConn::OK)
                        break;
    
                    
                    srvPath = (char *) malloc(strlen(currPath) +
                                        strlen(currComp->GetArchive()) + 1);
                    if (!srvPath)
                    {
                        err = E_MEM;
                        break;
                    }
                    sprintf(srvPath, "%s%s", currPath, currComp->GetArchive());

                    

                    isNewConn = CheckConn( currHost, TYPE_FTP, &myConn, PR_FALSE ); 
                    err = nsFTPConn::OK;

                    nsFTPConn *conn;
                    if ( isNewConn == PR_TRUE ) {
                      conn = new nsFTPConn(currHost, EventPumpCB);
                      if (!conn) {
                        err = E_MEM;
                        break;
                      }
                      err = conn->Open();
                      myConn.conn = (void *) conn;
                      myConn.type = TYPE_FTP;
                      myConn.URL = (char *) calloc(strlen(currHost) + 1, sizeof(char));
                      if ( myConn.URL != (char *) NULL )
                        strcpy( myConn.URL, currHost );
                    } else
                      conn = (nsFTPConn *) myConn.conn;

                    if (isNewConn == PR_FALSE || err == nsFTPConn::OK)
                    {
                        sprintf(localPath, "%s/%s", XPI_DIR,
                            currComp->GetArchive());
                        if (gCtx->opt->mMode == nsXIOptions::MODE_SILENT)
                          err = conn->Get(srvPath, localPath, nsFTPConn::BINARY, 
                              resPos, 1, NULL);
                        else
                          err = conn->Get(srvPath, localPath, nsFTPConn::BINARY, 
                              resPos, 1, nsInstallDlg::DownloadCB);
                    }

                    XI_IF_FREE(currHost);
                    XI_IF_FREE(currPath);
                    XI_IF_FREE(srvPath);
                }

                
                else
                {
                    err = nsHTTPConn::E_MALFORMED_URL;
                }

                if (err == nsHTTPConn::E_USER_CANCEL)
                    err = nsInstallDlg::CancelOrPause();

                
                if (err == nsInstallDlg::E_DL_PAUSE)
                {
                    currComp->SetResumePos(GetFileSize(localPath));
                    return err;
                }

                
                else if (err == nsInstallDlg::E_DL_CANCEL)
                    return err;

                
                else if (err != OK)
                {
                    fileSize = GetFileSize(localPath);

                    if (fileSize > 0)
                    {
                        
                        currComp->SetResumePos(fileSize);
                        return nsInstallDlg::E_DL_DROP_CXN;
                    }
                    else
                    {
                        
                        continue;
                    }
                }

                if (gCtx->opt->mMode != nsXIOptions::MODE_SILENT)
                    nsInstallDlg::ClearRateLabel(); 

                if (err == OK) 
                {
                    currComp->SetDownloaded(TRUE);
                    currCompNum++;
                    break;  
                }
            }
        }
        
        currComp = aComps->GetNext();
      }
   
      CheckConn( "", TYPE_UNDEF, &myConn, true );
 
      bDone = CRCCheckDownloadedArchives(XPI_DIR, strlen(XPI_DIR), 
                aComps, aCustom);
      crcPass++;
      if ( bDone == 0 && crcPass < MAXCRC ) {
        
        numToDL = TotalToDownload(aCustom, aComps);
        currComp = aComps->GetHead();
        currCompNum = 1;
        if (gCtx->opt->mMode != nsXIOptions::MODE_SILENT)
          gCtx->idlg->ReInitUI(); 
        gCtx->idlg->ShowCRCDlg(); 
      }
    }
    gCtx->idlg->DestroyCRCDlg(); 
    if ( crcPass < MAXCRC ) {
      
      DelDLMarker();
      return OK;
    } else {
      return E_CRC_FAILED;
    }
}






























PRBool
nsXIEngine::CheckConn( char *URL, int type, CONN *myConn, PRBool force )
{
	nsFTPConn *fconn;
	nsHTTPConn *hconn;
	PRBool retval = false;

	if ( myConn->type == TYPE_UNDEF )
		retval = PR_TRUE;					
	else if ( ( myConn->type != type || myConn->URL == (char *) NULL || strcmp( URL, myConn->URL ) || force == PR_TRUE ) ) {
		retval = PR_TRUE;
		switch ( myConn->type ) {
		case TYPE_HTTP:
		case TYPE_PROXY:
			hconn = (nsHTTPConn *) myConn->conn;
			hconn->Close();
			break;
		case TYPE_FTP:
			fconn = (nsFTPConn *) myConn->conn;
      if ( fconn != (nsFTPConn *) NULL ) {
        fconn->Close();
        XI_IF_DELETE(fconn);
        myConn->conn = NULL;
      }
			break;
		}
	}
	
	if ( retval == PR_TRUE && myConn->URL != (char *) NULL ) {
    free( myConn->URL );
    myConn->URL = (char *) NULL;
  }

	return retval;
}

int     
nsXIEngine::Extract(nsComponent *aXPIEngine)
{
    int rv;

    if (!aXPIEngine)
        return E_PARAM;

    mTmp = NULL;
    rv = MakeUniqueTmpDir();
    if (!mTmp || rv != OK)
        return E_DIR_CREATE;

    nsZipExtractor *unzip = new nsZipExtractor(XPI_DIR, mTmp);
    rv = unzip->Extract(aXPIEngine, CORE_LIB_COUNT);
    XI_IF_DELETE(unzip);

    return rv;
}

int     
nsXIEngine::Install(int aCustom, nsComponentList *aComps, char *aDestination)
{
    DUMP("Install");

    int err = OK;
    xpistub_t stub;
    char *old_LD_LIBRARY_PATH = NULL;
    char new_LD_LIBRARY_PATH[MAXPATHLEN];
    int i;
    int compNum = 1;
    int numComps;
    nsComponent *currComp = NULL;

    if (!aComps || !aDestination)
        return E_PARAM;

    numComps = aCustom ? aComps->GetLengthSelected() : aComps->GetLength();

    
#if defined (SOLARIS) || defined (IRIX)
    sprintf(new_LD_LIBRARY_PATH, "LD_LIBRARY_PATH=%s/bin:.", mTmp);
#else
    sprintf(new_LD_LIBRARY_PATH, "%s/bin:.", mTmp);
#endif
    DUMP(new_LD_LIBRARY_PATH);
    old_LD_LIBRARY_PATH = getenv("LD_LIBRARY_PATH");
#if defined (SOLARIS) || defined (IRIX)
    putenv(new_LD_LIBRARY_PATH);
#else
    setenv("LD_LIBRARY_PATH", new_LD_LIBRARY_PATH, 1);
#endif 
    currComp = aComps->GetHead();
    err = LoadXPIStub(&stub, aDestination);
    if (err == OK)
    {
        for (i = 0; i < MAX_COMPONENTS; i++)
        {
            if (!currComp)
                break;

            if (  (aCustom && currComp->IsSelected()) ||
                  (!aCustom)  )
            {
#ifdef DEBUG
                printf("%s %d: DOWNLOAD_ONLY for %s is %d\n", __FILE__, __LINE__, 
                    currComp->GetArchive(), currComp->IsDownloadOnly());
#endif
                if (!currComp->IsDownloadOnly())
                {
                    if (gCtx->opt->mMode != nsXIOptions::MODE_SILENT)
                        nsInstallDlg::MajorProgressCB(currComp->GetDescShort(),
                            compNum, numComps, nsInstallDlg::ACT_INSTALL);
                    err = InstallXPI(currComp, &stub);
                    if (err != OK)
                    if (err == E_INSTALL)
                        ErrorHandler(err, currComp->GetArchive()); 
                    else
                        ErrorHandler(err); 
                    compNum++;
                }
            }

            currComp = aComps->GetNext();
        }
        UnloadXPIStub(&stub);
    }

    
#if defined (SOLARIS) || defined (IRIX)
    char old_LD_env[MAXPATHLEN];

    sprintf(old_LD_env, "LD_LIBRARY_PATH=%s", old_LD_LIBRARY_PATH);
    putenv(old_LD_env);
#else
    setenv("LD_LIBRARY_PATH", old_LD_LIBRARY_PATH, 1);
#endif

    return err;
}

int
nsXIEngine::MakeUniqueTmpDir()
{
    int err = E_DIR_CREATE;
    char tmpnam[MAXPATHLEN];
    char *tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = getenv("TMP");
    if (!tmpdir) tmpdir = getenv("TEMP");
    if (!tmpdir) tmpdir = P_tmpdir;
    snprintf(tmpnam, sizeof(tmpnam), "%s/xpi.XXXXXX", tmpdir);
#ifdef HAVE_MKDTEMP
    if (mkdtemp(tmpnam)) {
      mTmp = strdup(tmpnam);
      if (mTmp) err = OK;
    }
#else
    int fd = mkstemp(tmpnam);
    if (fd < 0) return err;
    close(fd);
    if (unlink(tmpnam) < 0) return err;
    mTmp = strdup(tmpnam);
    if (!mTmp) return err;
    if (mkdir(mTmp, 0755) < 0) return err;
    err = OK;
#endif
    return err;
}

int
nsXIEngine::LoadXPIStub(xpistub_t *aStub, char *aDestination)
{
    int err = OK;

    char libpath[MAXPATHLEN];
    char libloc[MAXPATHLEN];
	char *dlerr;
    nsresult rv = 0;

	DUMP("LoadXPIStub");

    
    if (!aStub || !aDestination)
        return E_PARAM;

    
    mOriginalDir = (char *) malloc(MAXPATHLEN * sizeof(char));
    getcwd(mOriginalDir, MAXPATHLEN);

    
    sprintf(libloc, "%s/bin", mTmp);
    chdir(libloc);
    
    
    getcwd(libpath, MAXPATHLEN);
    if (strlen(libpath)+strlen(XPISTUB) + 2 > MAXPATHLEN) return E_MEM;
    strcat(libpath, "/");
    strcat(libpath, XPISTUB);

#ifdef DEBUG
printf("DEBUG: libpath = >>%s<<\n", libpath);
#endif

    aStub->handle = NULL;
    aStub->handle = dlopen(libpath, RTLD_LAZY);
    if (!aStub->handle)
    {
        dlerr = dlerror();
        DUMP(dlerr);
        ErrorHandler(E_LIB_OPEN, dlerr);
        return E_LIB_OPEN;
    }
    DUMP("xpistub opened");

    
    aStub->fn_init    = (pfnXPI_Init) dlsym(aStub->handle, FN_INIT);
    aStub->fn_install = (pfnXPI_Install) dlsym(aStub->handle, FN_INSTALL);
    aStub->fn_exit    = (pfnXPI_Exit) dlsym(aStub->handle, FN_EXIT);
    if (!aStub->fn_init || !aStub->fn_install || !aStub->fn_exit)
    {
        dlerr = dlerror();
        DUMP(dlerr);
        err = E_LIB_SYM;
        goto BAIL;
    }
    DUMP("xpistub symbols loaded");

    rv = aStub->fn_init(aDestination, NULL, ProgressCallback);

#ifdef DEBUG
printf("DEBUG: XPI_Init returned 0x%.8X\n", rv);
#endif

    DUMP("XPI_Init called");
	if (NS_FAILED(rv))
	{
		err = E_XPI_FAIL;
        goto BAIL;
	}

    return err;

BAIL:
    return err;
}

int
nsXIEngine::InstallXPI(nsComponent *aXPI, xpistub_t *aStub)
{
    int err = OK;
    char xpipath[MAXPATHLEN];
    nsresult rv = 0;

    if (!aStub || !aXPI || !mOriginalDir)
        return E_PARAM;

    sprintf(xpipath, "%s/%s/%s", mOriginalDir, XPI_DIR, aXPI->GetArchive());
    DUMP(xpipath);

#define XPI_NO_NEW_THREAD 0x1000

    rv = aStub->fn_install(xpipath, "", XPI_NO_NEW_THREAD);

#ifdef DEBUG
printf("DEBUG: XPI_Install %s returned %d\n", aXPI->GetArchive(), rv);
#endif

    if (NS_FAILED(rv))
        err = E_INSTALL;

    return err;
}

int
nsXIEngine::UnloadXPIStub(xpistub_t *aStub)
{
    int err = OK;

    
    if (!aStub)
        return E_PARAM;

	
    XI_ASSERT(aStub->fn_exit, "XPI_Exit is NULL and wasn't called!");
	if (aStub->fn_exit)
	{
		aStub->fn_exit();
		DUMP("XPI_Exit called");
	}

#if 0
    










	
	if (aStub->handle)
	{
		dlclose(aStub->handle);
		DUMP("xpistub closed");
	}
#endif

    return err;
}

void
nsXIEngine::ProgressCallback(const char* aMsg, PRInt32 aVal, PRInt32 aMax)
{
    
    
    nsInstallDlg::XPIProgressCB(aMsg, (int)aVal, (int)aMax);
}

int 
nsXIEngine::ExistAllXPIs(int aCustom, nsComponentList *aComps)
{
    DUMP("ExistAllXPIs");

    int bAllExist = TRUE;
    nsComponent *currComp = aComps->GetHead();
    char currArchivePath[256];
    struct stat dummy;

    while (currComp)
    {
        if ( (aCustom == TRUE && currComp->IsSelected()) || (aCustom == FALSE) )
        {
            sprintf(currArchivePath, "%s/%s", XPI_DIR, currComp->GetArchive());
            DUMP(currArchivePath);
            
            if (0 != stat(currArchivePath, &dummy)
                  || VerifyArchive(currArchivePath) != ZIP_OK)
                bAllExist = FALSE;
            else
                currComp->SetDownloaded(TRUE);
        }
        
        currComp = aComps->GetNext();
    }

    return bAllExist;
}

int
nsXIEngine::DeleteXPIs(int aCustom, nsComponentList *aComps)
{
    int err = OK;
    nsComponent *currComp = aComps->GetHead();
    char currXPIPath[MAXPATHLEN];

    while (currComp)
    {
        if ( (aCustom == TRUE && currComp->IsSelected()) || (aCustom == FALSE) )
        {
            sprintf(currXPIPath, "%s/%s/%s", mOriginalDir, XPI_DIR, 
                    currComp->GetArchive());
            
            
            err = unlink(currXPIPath);

#ifdef DEBUG
            printf("%s %d: unlink %s returned: %d\n", __FILE__, __LINE__, 
                currXPIPath, err); 
#endif
        }

        currComp = aComps->GetNext();
    }

    
    sprintf(currXPIPath, "%s/xpi", mOriginalDir);
    err = rmdir(currXPIPath);

#ifdef DEBUG
    printf("%s %d: rmdir %s returned: %d\n", __FILE__, __LINE__, 
        currXPIPath, err);
#endif
    
    return err;
}

int
nsXIEngine::GetFileSize(char *aPath)
{
    struct stat stbuf;

    if (!aPath)
        return 0;

    if (0 == stat(aPath, &stbuf))
    {
        return stbuf.st_size;
    }

    return 0;
}

int
nsXIEngine::SetDLMarker(char *aCompName)
{
    int rv = OK;
    FILE *dlMarkerFD;
    int compNameLen;

    if (!aCompName)
        return E_PARAM;

    
    dlMarkerFD = fopen(kDLMarkerPath, "w");
    if (!dlMarkerFD)
        return E_OPEN_MKR;

    
    compNameLen = strlen(aCompName);
    if (compNameLen > 0)
    {
        rv = fwrite((void *) aCompName, sizeof(char), compNameLen, dlMarkerFD);
        if (rv != compNameLen)
            rv = E_WRITE_MKR;
        else
            rv = OK;
    }

    
    fclose(dlMarkerFD);
        
#ifdef DEBUG
    printf("%s %d: SetDLMarker rv = %d\n", __FILE__, __LINE__, rv);
#endif
    return rv;
}

int
nsXIEngine::GetDLMarkedComp(nsComponentList *aComps, nsComponent **aOutComp)
{
    int rv = OK;
    FILE *dlMarkerFD = NULL;
    struct stat stbuf;
    char *compNameInFile = NULL;
    nsComponent *currComp = NULL;

    if (!aComps || !aOutComp)
        return E_PARAM;

    *aOutComp = NULL;
    currComp = aComps->GetHead();

    
    dlMarkerFD = fopen(kDLMarkerPath, "r");
    if (!dlMarkerFD)
        return E_OPEN_MKR;

    
    if (0 != stat(kDLMarkerPath, &stbuf))
    {
        rv = E_STAT;
        goto BAIL;
    }
    if (stbuf.st_size <= 0)
    {
        rv = E_FIND_COMP;
        goto BAIL;
    }

    
    compNameInFile = (char *) malloc(sizeof(char) * (stbuf.st_size + 1));
    if (!compNameInFile)
    {
        rv = E_MEM;
        goto BAIL;
    }
    memset(compNameInFile, 0 , (stbuf.st_size + 1));

    
    rv = fread((void *) compNameInFile, sizeof(char), 
               stbuf.st_size, dlMarkerFD);
    if (rv != stbuf.st_size)
        rv = E_READ_MKR;
    else
        rv = OK; 

    if (rv == OK)
    {
        
        while (currComp)
        {
            if (strcmp(currComp->GetArchive(), compNameInFile) == 0)
            {
                *aOutComp = currComp;
                break;
            }

            currComp = aComps->GetNext();
        }
    }

BAIL:
    if (dlMarkerFD)
        fclose(dlMarkerFD);

    XI_IF_FREE(compNameInFile);

    return rv;
}

int
nsXIEngine::DelDLMarker()
{
    return unlink(kDLMarkerPath);
}

int
nsXIEngine::TotalToDownload(int aCustom, nsComponentList *aComps)
{
    int total = 0;
    nsComponent *currComp;

    if (!aComps)
        return 0;

    currComp = aComps->GetHead();
    while (currComp)
    {
        if ( (aCustom == TRUE && currComp->IsSelected()) || (aCustom == FALSE) )
        {
            if (!currComp->IsDownloaded())
                total++;
        }
        currComp = aComps->GetNext();
    }

    return total;
}




















PRBool 
nsXIEngine::CRCCheckDownloadedArchives(char *dlPath, short dlPathlen, 
  nsComponentList *aComps, int aCustom)
{
  int i;
  PRBool isClean;
  char buf[ 1024 ];
  nsComponent *currComp = aComps->GetHead();
  int numComps = aCustom ? aComps->GetLengthSelected() : aComps->GetLength();

  isClean = PR_TRUE;

  for(i = 0; currComp != (nsComponent *) NULL && i < MAX_COMPONENTS; i++) {
    strncpy( buf, (const char *) dlPath, dlPathlen );
    buf[ dlPathlen ] = '\0';
    strcat( buf, "/" );
    strcat( buf, currComp->GetArchive() );
    if (gCtx->opt->mMode != nsXIOptions::MODE_SILENT) {
       nsInstallDlg::MajorProgressCB(buf, i, numComps, 
             nsInstallDlg::ACT_INSTALL);
    }
    if (((aCustom == TRUE && currComp->IsSelected()) || 
        (aCustom == FALSE)) && IsArchiveFile(buf) == TRUE && 
        VerifyArchive( buf ) != ZIP_OK) {
      currComp->SetDownloaded(FALSE); 
      isClean = PR_FALSE;
    }
    currComp = aComps->GetNext();
  }
  return isClean;
}























static char *extensions[] = { "ZIP", "XPI", "JAR" };  

PRBool
nsXIEngine::IsArchiveFile( char *buf ) 
{
    PRBool ret = false;
    char lbuf[1024];
    char *p;
    int i, max;
    
    
    
    if ( buf != (char *) NULL && ( p = strrchr( buf, '.' ) ) != (char *) NULL ) {
        p++;
        
        
        
        if ( *p != '\0' ) {
            
            
            

            strcpy( lbuf, p );
            for ( i = 0; i < (int) strlen( lbuf ); i++ )
            	lbuf[i] = toupper(lbuf[i]);
            
            
            	
            max = sizeof( extensions ) / sizeof ( char * );
            for ( i = 0; i < max; i++ ) 
                if ( !strcmp( lbuf, extensions[i] ) ) {
                    ret = true;
                    break;
                }
        }   
    }
    return ( ret );
}



















int
nsXIEngine::VerifyArchive(char *szArchive)
{
  void *vZip;
  int  iTestRv;
  char *penv;

  if((iTestRv = ZIP_OpenArchive(szArchive, &vZip)) == ZIP_OK)
  {
    
    
    iTestRv = ZIP_TestArchive(vZip);
    ZIP_CloseArchive(&vZip);
  }
 
  
  
  
 
  penv = getenv("MOZ_INSTALL_TEST_CRC");
  if ( penv != (char *) NULL ) { 
    if ( random() < RAND_MAX / 2 ) 
      iTestRv = !ZIP_OK;
  }

  if ( iTestRv != ZIP_OK )
    unlink( szArchive );
  return(iTestRv);
}

