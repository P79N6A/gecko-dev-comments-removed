








































#include "nsSystemPref.h"
#include "nsIObserverService.h"

#include "nsSystemPrefLog.h"
#include "nsSystemPrefService.h"
#include "nsString.h"

const char sSysPrefString[] = "config.use_system_prefs";
union MozPrefValue {
    char *      stringVal;
    PRInt32     intVal;
    PRBool      boolVal;
};

struct SysPrefItem {
    const char *prefName;       
    MozPrefValue defaultValue;  
    PRBool isLocked;  
    SysPrefItem() {
        prefName = nsnull;
        defaultValue.intVal = 0;
        defaultValue.stringVal = nsnull;
        defaultValue.boolVal = PR_FALSE;
        isLocked = PR_FALSE;
    }
    void SetPrefName(const char *aPrefName) {
        prefName = aPrefName;
    }
};


static const char *sSysPrefList[] = {
    "network.proxy.http",
    "network.proxy.http_port",
    "network.proxy.ftp",
    "network.proxy.ftp_port",
    "network.proxy.ssl",
    "network.proxy.ssl_port",
    "network.proxy.socks",
    "network.proxy.socks_port",
    "network.proxy.no_proxies_on",
    "network.proxy.autoconfig_url",
    "network.proxy.type",
    "config.use_system_prefs.accessibility",
};

PRLogModuleInfo *gSysPrefLog = NULL;

NS_IMPL_ISUPPORTS2(nsSystemPref, nsIObserver, nsISupportsWeakReference)

nsSystemPref::nsSystemPref():
    mSysPrefService(nsnull),
    mEnabled(PR_FALSE),
    mSysPrefs(nsnull)
{
}

nsSystemPref::~nsSystemPref()
{
    mSysPrefService = nsnull;
    mEnabled = PR_FALSE;
    delete [] mSysPrefs;
}





nsresult
nsSystemPref::Init(void)
{
    nsresult rv;

    if (!gSysPrefLog) {
        gSysPrefLog = PR_NewLogModule("Syspref");
        if (!gSysPrefLog)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIObserverService> observerService = 
        do_GetService("@mozilla.org/observer-service;1", &rv);

    if (observerService) {
        rv = observerService->AddObserver(this, NS_PREFSERVICE_READ_TOPIC_ID,
                                          PR_FALSE);
        rv = observerService->AddObserver(this, "profile-before-change",
                                          PR_FALSE);
        SYSPREF_LOG(("Add Observer for %s\n", NS_PREFSERVICE_READ_TOPIC_ID));
    }
    return(rv);
}





NS_IMETHODIMP
nsSystemPref::Observe(nsISupports *aSubject,
                      const char *aTopic,
                      const PRUnichar *aData)
{
    nsresult rv = NS_OK;

    if (!aTopic)
        return NS_OK;

    
    
    if (!nsCRT::strcmp(aTopic, NS_PREFSERVICE_READ_TOPIC_ID)) {
        SYSPREF_LOG(("Observed: %s\n", aTopic));

        nsCOMPtr<nsIPrefBranch2> prefBranch =
            do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv))
            return rv;

        rv = prefBranch->GetBoolPref(sSysPrefString, &mEnabled);
        if (NS_FAILED(rv)) {
            SYSPREF_LOG(("...FAil to Get %s\n", sSysPrefString));
            return rv;
        }

        
        mSysPrefService = do_GetService(NS_SYSTEMPREF_SERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv) || !mSysPrefService) {
            SYSPREF_LOG(("...No System Pref Service\n"));
            return NS_OK;
        }

        
        rv = prefBranch->AddObserver(sSysPrefString, this, PR_TRUE);
        if (NS_FAILED(rv)) {
            SYSPREF_LOG(("...FAil to add observer for %s\n", sSysPrefString));
            return rv;
        }

        if (!mEnabled) {
            SYSPREF_LOG(("%s is disabled\n", sSysPrefString));
            return NS_OK;
        }
        SYSPREF_LOG(("%s is enabled\n", sSysPrefString));
        rv = UseSystemPrefs();

    }
    
    else if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) &&
             NS_ConvertUTF8toUTF16(sSysPrefString).Equals(aData)) {
        SYSPREF_LOG(("++++++ Notify: topic=%s data=%s\n",
                     aTopic, NS_ConvertUTF16toUTF8(aData).get()));

        nsCOMPtr<nsIPrefBranch> prefBranch =
            do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv))
            return rv;

        PRBool enabled = mEnabled;
        rv = prefBranch->GetBoolPref(sSysPrefString, &mEnabled);
        if (enabled != mEnabled) {
            if (mEnabled)
                
                rv = UseSystemPrefs();
            else
                
                rv = UseMozillaPrefs();
        }
    }

    
    
    else if (!nsCRT::strcmp(aTopic, NS_SYSTEMPREF_PREFCHANGE_TOPIC_ID) &&
             aData) {
        NS_ASSERTION(mEnabled == PR_TRUE, "Should not listen when disabled");
        SYSPREF_LOG(("====== System Pref Notify topic=%s data=%s\n",
                     aTopic, (char*)aData));
        rv = ReadSystemPref(NS_LossyConvertUTF16toASCII(aData).get());
        return NS_OK;
    } else if (!nsCRT::strcmp(aTopic,"profile-before-change")) {
      
      if (mEnabled)
        UseMozillaPrefs();
      mEnabled = PR_FALSE;
      mSysPrefService = nsnull;
      delete [] mSysPrefs;
      mSysPrefs = nsnull;
    } else
        SYSPREF_LOG(("Not needed topic Received %s\n", aTopic));
    return rv;
}








nsresult
nsSystemPref::UseSystemPrefs()
{
    SYSPREF_LOG(("\n====Now Use system prefs==\n"));
    nsresult rv = NS_OK;
    if (!mSysPrefService) {
        return NS_ERROR_FAILURE;
    }

    PRIntn sysPrefCount= sizeof(sSysPrefList) / sizeof(sSysPrefList[0]);

    if (!mSysPrefs) {
        mSysPrefs = new SysPrefItem[sysPrefCount];
        if (!mSysPrefs)
            return NS_ERROR_OUT_OF_MEMORY;
        for (PRIntn index = 0; index < sysPrefCount; ++index)
            mSysPrefs[index].SetPrefName(sSysPrefList[index]);
    }

    for (PRIntn index = 0; index < sysPrefCount; ++index) {
        
        SaveMozDefaultPref(mSysPrefs[index].prefName,
                           &mSysPrefs[index].defaultValue,
                           &mSysPrefs[index].isLocked);

        
        ReadSystemPref(mSysPrefs[index].prefName);
        SYSPREF_LOG(("Add Listener on %s\n", mSysPrefs[index].prefName));
        mSysPrefService->AddObserver(mSysPrefs[index].prefName,
                                     this, PR_TRUE);
    }
    return rv;
}





nsresult
nsSystemPref::ReadSystemPref(const char *aPrefName)
{
    if (!mSysPrefService)
        return NS_ERROR_FAILURE;
    nsresult rv;

    nsCOMPtr<nsIPrefBranch> prefBranch
        (do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv))
        return rv;

    SYSPREF_LOG(("about to read aPrefName %s\n", aPrefName));

    prefBranch->UnlockPref(aPrefName);

    PRInt32 prefType = nsIPrefBranch::PREF_INVALID;
    nsXPIDLCString strValue;
    PRInt32 intValue = 0;
    PRBool boolValue = PR_FALSE;

    rv = prefBranch->GetPrefType(aPrefName, &prefType);
    if (NS_FAILED(rv))
        return rv;
    switch (prefType) {
    case nsIPrefBranch::PREF_STRING:
        mSysPrefService->GetCharPref(aPrefName, getter_Copies(strValue));
        SYSPREF_LOG(("system value is %s\n", strValue.get()));

        prefBranch->SetCharPref(aPrefName, strValue.get());
        break;
    case nsIPrefBranch::PREF_INT:
        mSysPrefService->GetIntPref(aPrefName, &intValue);
        SYSPREF_LOG(("system value is %d\n", intValue));

        prefBranch->SetIntPref(aPrefName, intValue);
        break;
    case nsIPrefBranch::PREF_BOOL:
        mSysPrefService->GetBoolPref(aPrefName, &boolValue);
        SYSPREF_LOG(("system value is %s\n", boolValue ? "TRUE" : "FALSE"));

        prefBranch->SetBoolPref(aPrefName, boolValue);
        break;
    default:
        SYSPREF_LOG(("Fail to system value for it\n"));
        return NS_ERROR_FAILURE;
    }
    prefBranch->LockPref(aPrefName);
    return NS_OK;
}





nsresult
nsSystemPref::UseMozillaPrefs()
{
    nsresult rv = NS_OK;
    SYSPREF_LOG(("\n====Now rollback to Mozilla prefs==\n"));

    
    if (!mSysPrefService)
        return NS_OK;

    PRIntn sysPrefCount= sizeof(sSysPrefList) / sizeof(sSysPrefList[0]);
    for (PRIntn index = 0; index < sysPrefCount; ++index) {
        
        RestoreMozDefaultPref(mSysPrefs[index].prefName,
                              &mSysPrefs[index].defaultValue,
                              mSysPrefs[index].isLocked);
        SYSPREF_LOG(("stop listening on %s\n", mSysPrefs[index].prefName));
        mSysPrefService->RemoveObserver(mSysPrefs[index].prefName,
                                        this);
    }
    return rv;
}







nsresult
nsSystemPref::SaveMozDefaultPref(const char *aPrefName,
                                 MozPrefValue *aPrefValue,
                                 PRBool *aLocked)
{
    NS_ENSURE_ARG_POINTER(aPrefName);
    NS_ENSURE_ARG_POINTER(aPrefValue);
    NS_ENSURE_ARG_POINTER(aLocked);

    nsresult rv;

    nsCOMPtr<nsIPrefBranch> prefBranch =
        do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    SYSPREF_LOG(("Save Mozilla value for %s\n", aPrefName));

    PRInt32 prefType = nsIPrefBranch::PREF_INVALID;
    nsXPIDLCString strValue;

    rv = prefBranch->GetPrefType(aPrefName, &prefType);
    if (NS_FAILED(rv))
        return rv;
    switch (prefType) {
    case nsIPrefBranch::PREF_STRING:
        prefBranch->GetCharPref(aPrefName,
                                getter_Copies(strValue));
        SYSPREF_LOG(("Mozilla value is %s", strValue.get()));

        if (aPrefValue->stringVal)
            PL_strfree(aPrefValue->stringVal);
        aPrefValue->stringVal = PL_strdup(strValue.get());
        break;
    case nsIPrefBranch::PREF_INT:
        prefBranch->GetIntPref(aPrefName, &aPrefValue->intVal);
        SYSPREF_LOG(("Mozilla value is %d\n", aPrefValue->intVal));

        break;
    case nsIPrefBranch::PREF_BOOL:
        prefBranch->GetBoolPref(aPrefName, &aPrefValue->boolVal);
        SYSPREF_LOG(("Mozilla value is %s\n",
                     aPrefValue->boolVal ? "TRUE" : "FALSE"));

        break;
    default:
        SYSPREF_LOG(("Fail to Read Mozilla value for it\n"));
        return NS_ERROR_FAILURE;
    }
    rv = prefBranch->PrefIsLocked(aPrefName, aLocked);
    SYSPREF_LOG((" (%s).\n", aLocked ? "Locked" : "NOT Locked"));
    return rv;
}







nsresult
nsSystemPref::RestoreMozDefaultPref(const char *aPrefName,
                                    MozPrefValue *aPrefValue,
                                    PRBool aLocked)
{
    NS_ENSURE_ARG_POINTER(aPrefName);

    nsresult rv;

    nsCOMPtr<nsIPrefBranch> prefBranch =
        do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    SYSPREF_LOG(("Restore Mozilla value for %s\n", aPrefName));

    PRInt32 prefType = nsIPrefBranch::PREF_INVALID;
    rv = prefBranch->GetPrefType(aPrefName, &prefType);
    if (NS_FAILED(rv))
        return rv;

    
    prefBranch->UnlockPref(aPrefName);

    switch (prefType) {
    case nsIPrefBranch::PREF_STRING:
        prefBranch->SetCharPref(aPrefName,
                                aPrefValue->stringVal);
        SYSPREF_LOG(("Mozilla value is %s\n", aPrefValue->stringVal));

        PL_strfree(aPrefValue->stringVal);
        aPrefValue->stringVal = nsnull;

        break;
    case nsIPrefBranch::PREF_INT:
        prefBranch->SetIntPref(aPrefName, aPrefValue->intVal);
        SYSPREF_LOG(("Mozilla value is %d\n", aPrefValue->intVal));

        break;
    case nsIPrefBranch::PREF_BOOL:
        prefBranch->SetBoolPref(aPrefName, aPrefValue->boolVal);
        SYSPREF_LOG(("Mozilla value is %s\n",
                     aPrefValue->boolVal ? "TRUE" : "FALSE"));

        break;
    default:
        SYSPREF_LOG(("Fail to Restore Mozilla value for it\n"));
        return NS_ERROR_FAILURE;
    }

    
    if (aLocked)
        prefBranch->LockPref(aPrefName);
    return NS_OK;
}
