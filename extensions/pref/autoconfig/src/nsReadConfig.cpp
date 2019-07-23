






































#ifdef MOZ_LOGGING

#define FORCE_PR_LOG
#endif
#include "nsReadConfig.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIAppStartup.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIAutoConfig.h"
#include "nsIComponentManager.h"
#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPromptService.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsXPFEComponentsCID.h"
#include "nsXPIDLString.h"
#include "nsNetUtil.h"
#include "prmem.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nspr.h"

extern PRLogModuleInfo *MCD;

extern nsresult EvaluateAdminConfigScript(const char *js_buffer, size_t length,
                                          const char *filename, 
                                          PRBool bGlobalContext, 
                                          PRBool bCallbacks, 
                                          PRBool skipFirstLine);
extern nsresult CentralizedAdminPrefManagerInit();
extern nsresult CentralizedAdminPrefManagerFinish();


static void DisplayError(void)
{
    nsresult rv;

    nsCOMPtr<nsIPromptService> promptService = do_GetService("@mozilla.org/embedcomp/prompt-service;1");
    if (!promptService)
        return;

    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    if (!bundleService)
        return;

    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle("chrome://autoconfig/locale/autoconfig.properties",
                                getter_AddRefs(bundle));
    if (!bundle)
        return;

    nsXPIDLString title;
    rv = bundle->GetStringFromName(NS_LITERAL_STRING("readConfigTitle").get(), getter_Copies(title));
    if (NS_FAILED(rv))
        return;

    nsXPIDLString err;
    rv = bundle->GetStringFromName(NS_LITERAL_STRING("readConfigMsg").get(), getter_Copies(err));
    if (NS_FAILED(rv))
        return;

    promptService->Alert(nsnull, title.get(), err.get());
}



NS_IMPL_THREADSAFE_ISUPPORTS2(nsReadConfig, nsIReadConfig, nsIObserver)

nsReadConfig::nsReadConfig() :
    mRead(PR_FALSE)
{
    if (!MCD)
      MCD = PR_NewLogModule("MCD");
}

nsresult nsReadConfig::Init()
{
    nsresult rv;
    
    nsCOMPtr<nsIObserverService> observerService = 
        do_GetService("@mozilla.org/observer-service;1", &rv);

    if (observerService) {
        rv = observerService->AddObserver(this, NS_PREFSERVICE_READ_TOPIC_ID, PR_FALSE);
    }
    return(rv);
}

nsReadConfig::~nsReadConfig()
{
    CentralizedAdminPrefManagerFinish();
}

NS_IMETHODIMP nsReadConfig::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;

    if (!nsCRT::strcmp(aTopic, NS_PREFSERVICE_READ_TOPIC_ID)) {
        rv = readConfigFile();
        if (NS_FAILED(rv)) {
            DisplayError();

            nsCOMPtr<nsIAppStartup> appStartup =
                do_GetService(NS_APPSTARTUP_CONTRACTID);
            if (appStartup)
                appStartup->Quit(nsIAppStartup::eAttemptQuit);
        }
    }
    return rv;
}


nsresult nsReadConfig::readConfigFile()
{
    nsresult rv = NS_OK;
    nsXPIDLCString lockFileName;
    nsXPIDLCString lockVendor;
    PRUint32 fileNameLen = 0;
    
    nsCOMPtr<nsIPrefBranch> defaultPrefBranch;
    nsCOMPtr<nsIPrefService> prefService = 
        do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    rv = prefService->GetDefaultBranch(nsnull, getter_AddRefs(defaultPrefBranch));
    if (NS_FAILED(rv))
        return rv;
        
    
    

    rv = defaultPrefBranch->GetCharPref("general.config.filename", 
                                  getter_Copies(lockFileName));


    PR_LOG(MCD, PR_LOG_DEBUG, ("general.config.filename = %s\n", lockFileName.get()));
    if (NS_FAILED(rv))
        return rv;

    
    
    if (!mRead) {
        
        
        rv = CentralizedAdminPrefManagerInit();
        if (NS_FAILED(rv))
            return rv;
        
        
        rv = openAndEvaluateJSFile("prefcalls.js", 0, PR_FALSE, PR_FALSE);
        if (NS_FAILED(rv)) 
            return rv;

        
        rv = openAndEvaluateJSFile("platform.js", 0, PR_FALSE, PR_FALSE);
        if (NS_FAILED(rv)) 
            return rv;

        mRead = PR_TRUE;
    }
    
  
  
    
    
    
    

    nsCOMPtr<nsIPrefBranch> prefBranch;
    rv = prefService->GetBranch(nsnull, getter_AddRefs(prefBranch));
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 obscureValue = 0;
    (void) defaultPrefBranch->GetIntPref("general.config.obscure_value", &obscureValue);
    PR_LOG(MCD, PR_LOG_DEBUG, ("evaluating .cfg file %s with obscureValue %d\n", lockFileName.get(), obscureValue));
    rv = openAndEvaluateJSFile(lockFileName.get(), PR_TRUE, obscureValue, PR_TRUE);
    if (NS_FAILED(rv))
    {
      PR_LOG(MCD, PR_LOG_DEBUG, ("error evaluating .cfg file %s %x\n", lockFileName.get(), rv));
      return rv;
    }
    
    rv = prefBranch->GetCharPref("general.config.filename", 
                                  getter_Copies(lockFileName));
    if (NS_FAILED(rv))
        
        
        return NS_ERROR_FAILURE;

  
    rv = prefBranch->GetCharPref("general.config.vendor", 
                                  getter_Copies(lockVendor));
    
    if (NS_SUCCEEDED(rv)) {

        fileNameLen = PL_strlen(lockFileName);
    
        
        
        
    
        if (PL_strncmp(lockFileName, lockVendor, fileNameLen - 4) != 0)
            return NS_ERROR_FAILURE;
    }
  
    
    nsXPIDLCString urlName;
    rv = prefBranch->GetCharPref("autoadmin.global_config_url",
                                  getter_Copies(urlName));
    if (NS_SUCCEEDED(rv) && !urlName.IsEmpty()) {

        
        mAutoConfig = do_CreateInstance(NS_AUTOCONFIG_CONTRACTID, &rv);
        if (NS_FAILED(rv))
            return NS_ERROR_OUT_OF_MEMORY;

        rv = mAutoConfig->SetConfigURL(urlName);
        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;

    }
  
    return NS_OK;
} 


nsresult nsReadConfig::openAndEvaluateJSFile(const char *aFileName, PRBool isEncoded, 
                                             PRInt32 obscureValue,
                                             PRBool isBinDir)
{
    nsresult rv;
    nsCOMPtr<nsIFile> jsFile;

    if (isBinDir) {
        rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR, 
                                    getter_AddRefs(jsFile));
        if (NS_FAILED(rv)) 
            return rv;
        
#ifdef XP_MAC
        jsFile->AppendNative(NS_LITERAL_CSTRING("Essential Files"));
#endif
    } else {
        rv = NS_GetSpecialDirectory(NS_APP_DEFAULTS_50_DIR,
                                    getter_AddRefs(jsFile));
        if (NS_FAILED(rv)) 
            return rv;
        rv = jsFile->AppendNative(NS_LITERAL_CSTRING("autoconfig"));
        if (NS_FAILED(rv))
            return rv;
    }
    rv = jsFile->AppendNative(nsDependentCString(aFileName));
    if (NS_FAILED(rv)) 
        return rv;

    nsCOMPtr<nsIInputStream> inStr;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inStr), jsFile);
    if (NS_FAILED(rv)) 
        return rv;        
        
    PRInt64 fileSize;
    PRUint32 fs, amt = 0;
    jsFile->GetFileSize(&fileSize);
    LL_L2UI(fs, fileSize); 

    char *buf = (char *)PR_Malloc(fs * sizeof(char));
    if (!buf) 
        return NS_ERROR_OUT_OF_MEMORY;
      
    rv = inStr->Read(buf, fs, &amt);
    NS_ASSERTION((amt == fs), "failed to read the entire configuration file!!");
    if (NS_SUCCEEDED(rv)) {
        if (obscureValue > 0) {

            
            for (PRUint32 i = 0; i < amt; i++)
                buf[i] -= obscureValue;
        }
        nsCAutoString path;

        jsFile->GetNativePath(path);
        nsCAutoString fileURL;
        fileURL = NS_LITERAL_CSTRING("file:///") + path;
        rv = EvaluateAdminConfigScript(buf, amt, fileURL.get(), 
                                       PR_FALSE, PR_TRUE, 
                                       isEncoded ? PR_TRUE:PR_FALSE);
    }
    inStr->Close();
    PR_Free(buf);
    
    return rv;
}
