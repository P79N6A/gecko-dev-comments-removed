







































#include "xpistub.h"

#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsLocalFile.h"

#include "nscore.h"
#include "nspr.h"

#include "nsStubNotifier.h"

#include "nsISoftwareUpdate.h"
#include "nsSoftwareUpdateIIDs.h"
#include "nsPIXPIStubHook.h"

#include "plstr.h"

#if defined(XP_WIN) || defined(XP_OS2)
#if defined(XP_OS2)
#include <stdlib.h>
#define MAX_PATH _MAX_PATH
#else
#include <windows.h>
#endif
#ifndef XP_OS2_EMX
#include <direct.h>
#endif 
#define COMPONENT_REG "component.reg"
#endif

#ifdef XP_UNIX
#include <unistd.h>
#define COMPONENT_REG "component.reg"
#endif





static nsIXPIListener      *gListener = 0;
static nsISoftwareUpdate   *gXPI = 0;
static nsIServiceManager   *gServiceMgr = 0;

static NS_DEFINE_IID(kSoftwareUpdateCID, NS_SoftwareUpdate_CID);

PRInt32 gInstallStatus;






PR_PUBLIC_API(nsresult) XPI_Init( const char*         aProgramDir,
                                  const char*         aLogName,
                                  pfnXPIProgress      progressCB )
{
    nsresult              rv;

    
    
    
#if defined(XP_WIN) || defined(XP_OS2)

 #ifdef XP_OS2_EMX
    char componentPath[MAX_PATH];
    _getcwd2(componentPath, MAX_PATH);
    int len = strlen(componentPath);
    for (int i = 0; i < len; i++) {
      if (componentPath[i] == '/') {
        componentPath[i] = '\\';
      }
    }
 #else
    char componentPath[MAX_PATH];
    getcwd(componentPath, MAX_PATH);
 #endif

    nsCOMPtr<nsILocalFile> file;
    rv = NS_NewNativeLocalFile(nsDependentCString(componentPath), PR_TRUE, getter_AddRefs(file));
    if (NS_FAILED(rv)) return rv;
    
    rv = NS_InitXPCOM2(&gServiceMgr, file, nsnull); 

#elif defined(XP_UNIX)

    rv = NS_InitXPCOM2(&gServiceMgr, nsnull, nsnull); 

    char cwd[1024];
    char compDirPath[1024];

    memset(cwd, 0, 1024);
    memset(compDirPath, 0, 1024);
    getcwd(cwd, 1024);
    sprintf(compDirPath, "%s/components", cwd);

    nsCOMPtr<nsILocalFile> compDir;
    NS_NewNativeLocalFile(nsDependentCString(compDirPath), PR_TRUE, getter_AddRefs(compDir));

#else

    rv = NS_InitXPCOM2(&gServiceMgr, NULL, NULL);

#endif

    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(gServiceMgr);
    NS_ASSERTION(registrar, "Null nsIComponentRegistrar");

#if defined(XP_UNIX)
    rv = registrar->AutoRegister(compDir);
#else
    rv = registrar->AutoRegister(nsnull);
#endif
    if (NS_FAILED(rv))
        return rv;


    
    
    
    
    
    
    
    rv = CallCreateInstance(kSoftwareUpdateCID, &gXPI);
    if (NS_FAILED(rv))
        return rv;


    
    
    
    
    nsCOMPtr<nsPIXPIStubHook>   hook = do_QueryInterface(gXPI);
    nsCOMPtr<nsILocalFile>      iDirSpec;
  
    if (aProgramDir)
    {
	NS_NewNativeLocalFile(nsDependentCString(aProgramDir), PR_TRUE, getter_AddRefs(iDirSpec));
    }
    else
    {
       nsCOMPtr<nsIProperties> directoryService = 
          do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);

       directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsILocalFile), 
                             getter_AddRefs(iDirSpec));
    }

    if (hook && iDirSpec)
    {
        rv = hook->StubInitialize( iDirSpec, aLogName );
        if (NS_FAILED(rv)) return rv;
    }
    else
        return NS_ERROR_NULL_POINTER;


    
    
    
    nsStubListener* stub = new nsStubListener( progressCB );
    if (!stub)
    {
        gXPI->Release();
        rv = NS_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        rv = stub->QueryInterface(NS_GET_IID(nsIXPIListener), (void**)&gListener);
    }
    return rv;
}






PR_PUBLIC_API(void) XPI_Exit()
{
    if (gListener)
        gListener->Release();

    if (gXPI)
        gXPI->Release();

    NS_ShutdownXPCOM(gServiceMgr);

}







PR_PUBLIC_API(PRInt32) XPI_Install(
                                    const char*   aFile,
                                    const char*   aArgs, 
                                    long          aFlags )
{
    nsresult                rv = NS_ERROR_NULL_POINTER;
    nsString                args; args.AssignWithConversion(aArgs);
    nsCOMPtr<nsILocalFile>  iFile;

    gInstallStatus = -322; 
    
    NS_NewNativeLocalFile(nsDependentCString(aFile), PR_TRUE, getter_AddRefs(iFile));

    if (iFile && gXPI)
        rv = gXPI->InstallJar( iFile,
                               nsnull,
                               args.get(),
                               nsnull,
                               (aFlags | XPI_NO_NEW_THREAD),
                               gListener );

    return gInstallStatus;
}
