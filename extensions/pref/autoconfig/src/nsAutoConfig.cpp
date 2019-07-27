




#ifdef MOZ_LOGGING

#define FORCE_PR_LOG
#endif
#include "nsAutoConfig.h"
#include "nsIURI.h"
#include "nsIHttpChannel.h"
#include "nsIFileStreams.h"
#include "nsThreadUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "prmem.h"
#include "nsIObserverService.h"
#include "nsLiteralString.h"
#include "nsIPromptService.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsContentUtils.h"
#include "nsCRT.h"
#include "nspr.h"
#include <algorithm>

PRLogModuleInfo *MCD;

extern nsresult EvaluateAdminConfigScript(const char *js_buffer, size_t length,
                                          const char *filename, 
                                          bool bGlobalContext, 
                                          bool bCallbacks, 
                                          bool skipFirstLine);



NS_IMPL_ISUPPORTS(nsAutoConfig, nsIAutoConfig, nsITimerCallback, nsIStreamListener, nsIObserver, nsIRequestObserver, nsISupportsWeakReference)

nsAutoConfig::nsAutoConfig()
{
}

nsresult nsAutoConfig::Init()
{
    

    nsresult rv;
    mLoaded = false;
    
    
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_FAILED(rv)) 
        return rv;

    rv = observerService->AddObserver(this,"profile-after-change", true);
    
    return rv;
}

nsAutoConfig::~nsAutoConfig()
{
}


NS_IMETHODIMP nsAutoConfig::GetConfigURL(char **aConfigURL)
{
    if (!aConfigURL) 
        return NS_ERROR_NULL_POINTER;

    if (mConfigURL.IsEmpty()) {
        *aConfigURL = nullptr;
        return NS_OK;
    }
    
    *aConfigURL = ToNewCString(mConfigURL);
    if (!*aConfigURL)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}
NS_IMETHODIMP nsAutoConfig::SetConfigURL(const char *aConfigURL)
{
    if (!aConfigURL)
        return NS_ERROR_NULL_POINTER;
    mConfigURL.Assign(aConfigURL);
    return NS_OK;
}

NS_IMETHODIMP
nsAutoConfig::OnStartRequest(nsIRequest *request, nsISupports *context)
{
    return NS_OK;
}


NS_IMETHODIMP
nsAutoConfig::OnDataAvailable(nsIRequest *request, 
                              nsISupports *context,
                              nsIInputStream *aIStream, 
                              uint64_t aSourceOffset,
                              uint32_t aLength)
{    
    uint32_t amt, size;
    nsresult rv;
    char buf[1024];
    
    while (aLength) {
        size = std::min<size_t>(aLength, sizeof(buf));
        rv = aIStream->Read(buf, size, &amt);
        if (NS_FAILED(rv))
            return rv;
        mBuf.Append(buf, amt);
        aLength -= amt;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsAutoConfig::OnStopRequest(nsIRequest *request, nsISupports *context,
                            nsresult aStatus)
{
    nsresult rv;

    
    if (NS_FAILED(aStatus)) {
        PR_LOG(MCD, PR_LOG_DEBUG, ("mcd request failed with status %x\n", aStatus));
        return readOfflineFile();
    }

    
    nsCOMPtr<nsIHttpChannel> pHTTPCon(do_QueryInterface(request));
    if (pHTTPCon) {
        uint32_t httpStatus;
        pHTTPCon->GetResponseStatus(&httpStatus);
        if (httpStatus != 200) 
        {
            PR_LOG(MCD, PR_LOG_DEBUG, ("mcd http request failed with status %x\n", httpStatus));
            return readOfflineFile();
        }
    }
    
    
    
    rv = EvaluateAdminConfigScript(mBuf.get(), mBuf.Length(),
                              nullptr, false,true, false);
    if (NS_SUCCEEDED(rv)) {

        
        rv = writeFailoverFile(); 

        if (NS_FAILED(rv)) 
            NS_WARNING("Error writing failover.jsc file");

        
        mLoaded = true;  

        return NS_OK;
    }
    
    NS_WARNING("Error reading autoconfig.jsc from the network, reading the offline version");
    return readOfflineFile();
}


NS_IMETHODIMP nsAutoConfig::Notify(nsITimer *timer) 
{
    downloadAutoConfig();
    return NS_OK;
}






NS_IMETHODIMP nsAutoConfig::Observe(nsISupports *aSubject, 
                                    const char *aTopic, 
                                    const char16_t *someData)
{
    nsresult rv = NS_OK;
    if (!nsCRT::strcmp(aTopic, "profile-after-change")) {

        
        
        
        
        rv = downloadAutoConfig();

    }  
   
    return rv;
}

nsresult nsAutoConfig::downloadAutoConfig()
{
    nsresult rv;
    nsAutoCString emailAddr;
    nsXPIDLCString urlName;
    static bool firstTime = true;
    
    if (mConfigURL.IsEmpty()) {
        PR_LOG(MCD, PR_LOG_DEBUG, ("global config url is empty - did you set autoadmin.global_config_url?\n"));
        NS_WARNING("AutoConfig called without global_config_url");
        return NS_OK;
    }
    
    
    
    
    
    int32_t index = mConfigURL.RFindChar((char16_t)'?');
    if (index != -1)
        mConfigURL.Truncate(index);

    
    if (!mBuf.IsEmpty())
        mBuf.Truncate(0);

    
    if (!mPrefBranch) {
        nsCOMPtr<nsIPrefService> prefs =
            do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) 
            return rv;
    
        rv = prefs->GetBranch(nullptr,getter_AddRefs(mPrefBranch));
        if (NS_FAILED(rv))
            return rv;
    }
    
    
    nsCOMPtr<nsIIOService> ios = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) 
        return rv;
    
    bool offline;
    rv = ios->GetOffline(&offline);
    if (NS_FAILED(rv)) 
        return rv;
    
    if (offline) {
        bool offlineFailover;
        rv = mPrefBranch->GetBoolPref("autoadmin.offline_failover", 
                                      &offlineFailover);
        
        if (NS_SUCCEEDED(rv) && offlineFailover)
            return readOfflineFile();
    }

    




    bool appendMail;
    rv = mPrefBranch->GetBoolPref("autoadmin.append_emailaddr", &appendMail);
    if (NS_SUCCEEDED(rv) && appendMail) {
        rv = getEmailAddr(emailAddr);
        if (NS_SUCCEEDED(rv) && emailAddr.get()) {
            



            mConfigURL.Append('?');
            mConfigURL.Append(emailAddr); 
        }
    }
    
    
    nsCOMPtr<nsIURI> url;
    nsCOMPtr<nsIChannel> channel;
    
    rv = NS_NewURI(getter_AddRefs(url), mConfigURL.get(), nullptr, nullptr);
    if (NS_FAILED(rv))
    {
        PR_LOG(MCD, PR_LOG_DEBUG, ("failed to create URL - is autoadmin.global_config_url valid? - %s\n", mConfigURL.get()));
        return rv;
    }

    PR_LOG(MCD, PR_LOG_DEBUG, ("running MCD url %s\n", mConfigURL.get()));
    
    rv = NS_NewChannel(getter_AddRefs(channel),
                       url,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER,
                       nullptr,  
                       nullptr,  
                       nullptr,  
                       nsIRequest::INHIBIT_PERSISTENT_CACHING |
                       nsIRequest::LOAD_BYPASS_CACHE);

    if (NS_FAILED(rv)) 
        return rv;

    rv = channel->AsyncOpen(this, nullptr); 
    if (NS_FAILED(rv)) {
        readOfflineFile();
        return rv;
    }
    
    
    
    
    
    if (firstTime) {
        firstTime = false;
    
        
        

        nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
        NS_ENSURE_STATE(thread);
    
        






        
        while (!mLoaded)
            NS_ENSURE_STATE(NS_ProcessNextEvent(thread));
        
        int32_t minutes;
        rv = mPrefBranch->GetIntPref("autoadmin.refresh_interval", 
                                     &minutes);
        if (NS_SUCCEEDED(rv) && minutes > 0) {
            
            
            mTimer = do_CreateInstance("@mozilla.org/timer;1",&rv);
            if (NS_FAILED(rv)) 
                return rv;
            rv = mTimer->InitWithCallback(this, minutes * 60 * 1000, 
                             nsITimer::TYPE_REPEATING_SLACK);
            if (NS_FAILED(rv)) 
                return rv;
        }
    } 
    
    return NS_OK;
} 



nsresult nsAutoConfig::readOfflineFile()
{
    nsresult rv;
    
    



    mLoaded = true; 

    bool failCache;
    rv = mPrefBranch->GetBoolPref("autoadmin.failover_to_cached", &failCache);
    if (NS_SUCCEEDED(rv) && !failCache) {
        
        
        nsCOMPtr<nsIIOService> ios =
            do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) 
            return rv;
        
        bool offline;
        rv = ios->GetOffline(&offline);
        if (NS_FAILED(rv)) 
            return rv;

        if (!offline) {
            rv = ios->SetOffline(true);
            if (NS_FAILED(rv)) 
                return rv;
        }
        
        
        
        rv = mPrefBranch->SetBoolPref("network.online", false);
        if (NS_FAILED(rv)) 
            return rv;

        mPrefBranch->LockPref("network.online");
        return NS_OK;
    }
    
    



    
    nsCOMPtr<nsIFile> failoverFile; 
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(failoverFile));
    if (NS_FAILED(rv)) 
        return rv;
    
    failoverFile->AppendNative(NS_LITERAL_CSTRING("failover.jsc"));
    rv = evaluateLocalFile(failoverFile);
    if (NS_FAILED(rv)) 
        NS_WARNING("Couldn't open failover.jsc, going back to default prefs");
    return NS_OK;
}

nsresult nsAutoConfig::evaluateLocalFile(nsIFile *file)
{
    nsresult rv;
    nsCOMPtr<nsIInputStream> inStr;
    
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inStr), file);
    if (NS_FAILED(rv)) 
        return rv;
        
    int64_t fileSize;
    file->GetFileSize(&fileSize);
    uint32_t fs = fileSize; 
    char *buf = (char *)PR_Malloc(fs * sizeof(char));
    if (!buf) 
        return NS_ERROR_OUT_OF_MEMORY;

    uint32_t amt = 0;
    rv = inStr->Read(buf, fs, &amt);
    if (NS_SUCCEEDED(rv)) {
      EvaluateAdminConfigScript(buf, fs, nullptr, false, 
                                true, false);
    }
    inStr->Close();
    PR_Free(buf);
    return rv;
}

nsresult nsAutoConfig::writeFailoverFile()
{
    nsresult rv;
    nsCOMPtr<nsIFile> failoverFile; 
    nsCOMPtr<nsIOutputStream> outStr;
    uint32_t amt;
    
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(failoverFile));
    if (NS_FAILED(rv)) 
        return rv;
    
    failoverFile->AppendNative(NS_LITERAL_CSTRING("failover.jsc"));
    
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(outStr), failoverFile);
    if (NS_FAILED(rv)) 
        return rv;
    rv = outStr->Write(mBuf.get(),mBuf.Length(),&amt);
    outStr->Close();
    return rv;
}

nsresult nsAutoConfig::getEmailAddr(nsACString & emailAddr)
{
    
    nsresult rv;
    nsXPIDLCString prefValue;
    
    





    
    rv = mPrefBranch->GetCharPref("mail.accountmanager.defaultaccount", 
                                  getter_Copies(prefValue));
    if (NS_SUCCEEDED(rv) && !prefValue.IsEmpty()) {
        emailAddr = NS_LITERAL_CSTRING("mail.account.") +
            prefValue + NS_LITERAL_CSTRING(".identities");
        rv = mPrefBranch->GetCharPref(PromiseFlatCString(emailAddr).get(),
                                      getter_Copies(prefValue));
        if (NS_FAILED(rv) || prefValue.IsEmpty())
            return PromptForEMailAddress(emailAddr);
        int32_t commandIndex = prefValue.FindChar(',');
        if (commandIndex != kNotFound)
          prefValue.Truncate(commandIndex);
        emailAddr = NS_LITERAL_CSTRING("mail.identity.") +
            prefValue + NS_LITERAL_CSTRING(".useremail");
        rv = mPrefBranch->GetCharPref(PromiseFlatCString(emailAddr).get(),
                                      getter_Copies(prefValue));
        if (NS_FAILED(rv)  || prefValue.IsEmpty())
            return PromptForEMailAddress(emailAddr);
        emailAddr = prefValue;
    }
    else {
        
        rv = mPrefBranch->GetCharPref("mail.identity.useremail", 
                                  getter_Copies(prefValue));
        if (NS_SUCCEEDED(rv) && !prefValue.IsEmpty())
            emailAddr = prefValue;
        else
            PromptForEMailAddress(emailAddr);
    }
    
    return NS_OK;
}
        
nsresult nsAutoConfig::PromptForEMailAddress(nsACString &emailAddress)
{
    nsresult rv;
    nsCOMPtr<nsIPromptService> promptService = do_GetService("@mozilla.org/embedcomp/prompt-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleService->CreateBundle("chrome://autoconfig/locale/autoconfig.properties",
                                getter_AddRefs(bundle));
    NS_ENSURE_SUCCESS(rv, rv);

    nsXPIDLString title;
    rv = bundle->GetStringFromName(MOZ_UTF16("emailPromptTitle"), getter_Copies(title));
    NS_ENSURE_SUCCESS(rv, rv);

    nsXPIDLString err;
    rv = bundle->GetStringFromName(MOZ_UTF16("emailPromptMsg"), getter_Copies(err));
    NS_ENSURE_SUCCESS(rv, rv);
    bool check = false;
    nsXPIDLString emailResult;
    bool success;
    rv = promptService->Prompt(nullptr, title.get(), err.get(), getter_Copies(emailResult), nullptr, &check, &success);
    if (!success)
      return NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(rv, rv);
    LossyCopyUTF16toASCII(emailResult, emailAddress);
    return NS_OK;
}
