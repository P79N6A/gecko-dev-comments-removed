









































#define INCL_DOS
#include <os2.h>

#include "nsMIMEInfoOS2.h"
#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsReadableUtils.h"
#include "nsIProcess.h"
#include <stdlib.h>

#define SALT_SIZE 8
#define TABLE_SIZE 36
static const PRUnichar table[] = 
  { 'a','b','c','d','e','f','g','h','i','j',
    'k','l','m','n','o','p','q','r','s','t',
    'u','v','w','x','y','z','0','1','2','3',
    '4','5','6','7','8','9'};


nsMIMEInfoOS2::~nsMIMEInfoOS2()
{
}

NS_IMETHODIMP nsMIMEInfoOS2::LaunchWithFile(nsIFile* aFile)
{
  nsresult rv = NS_OK;

  nsCAutoString path;
  aFile->GetNativePath(path);

  nsCOMPtr<nsIFile> application;
  if (mPreferredAction == useHelperApp) {
    nsCOMPtr<nsILocalHandlerApp> localHandlerApp =
      do_QueryInterface(mPreferredApplication, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = localHandlerApp->GetExecutable(getter_AddRefs(application));
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mPreferredAction == useSystemDefault) {
    application = mDefaultApplication;
  } else {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  if (!application) {
    HOBJECT hobject = WinQueryObject(path.get());
    if (WinSetObjectData( hobject, "OPEN=DEFAULT" ))
      return NS_OK;
    else
      return NS_ERROR_FAILURE;
  }
  
  ULONG ulAppType;
  nsCAutoString apppath;
  application->GetNativePath(apppath);
  DosQueryAppType(apppath.get(), &ulAppType);
  if (ulAppType & (FAPPTYP_DOS |
                   FAPPTYP_WINDOWSPROT31 |
                   FAPPTYP_WINDOWSPROT |
                   FAPPTYP_WINDOWSREAL)) {
    
    
    nsCOMPtr<nsPIExternalAppLauncher> helperAppService (do_GetService(NS_EXTERNALHELPERAPPSERVICE_CONTRACTID));
    if (helperAppService)
    {
      nsCAutoString leafName; 
      aFile->GetNativeLeafName(leafName);
      const char* lastDot = strrchr(leafName.get(), '.');
      char suffix[CCHMAXPATH + 1] = "";
      if (lastDot)
      {
          strcpy(suffix, lastDot);
      }
      suffix[4] = '\0';
      
      nsAutoString saltedTempLeafName;
      do {
          saltedTempLeafName.Truncate();
          
          
          double fpTime;
          LL_L2D(fpTime, PR_Now());
          srand((uint)(fpTime * 1e-6 + 0.5));
          PRInt32 i;
          for (i=0;i<SALT_SIZE;i++) {
            saltedTempLeafName.Append(table[(rand()%TABLE_SIZE)]);
          }
          AppendASCIItoUTF16(suffix, saltedTempLeafName);
          rv = aFile->MoveTo(nsnull, saltedTempLeafName);
      } while (NS_FAILED(rv));
      helperAppService->DeleteTemporaryFileOnExit(aFile);
      aFile->GetNativePath(path);
    }
  } else {
    path.Insert('\"', 0);
    path.Append('\"');
  }
    
  const char * strPath = path.get();
  
  
  nsCOMPtr<nsIProcess> process = do_CreateInstance(NS_PROCESS_CONTRACTID);
  if (NS_FAILED(rv = process->Init(application)))
    return rv;
  PRUint32 pid;
  return process->Run(PR_FALSE, &strPath, 1, &pid);
}

