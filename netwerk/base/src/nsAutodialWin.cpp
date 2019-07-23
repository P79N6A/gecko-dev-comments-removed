










































#include <windows.h>
#include <winsvc.h>
#include "nsString.h"
#include "nsAutodialWin.h"
#include "prlog.h"

#define AUTODIAL_DEFAULT AUTODIAL_NEVER













#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = nsnull;
#endif

#define LOGD(args) PR_LOG(gLog, PR_LOG_DEBUG, args)
#define LOGE(args) PR_LOG(gLog, PR_LOG_ERROR, args)


#define NO_RETRY_PERIOD_SEC 5
PRIntervalTime nsAutodial::mDontRetryUntil = 0;


tRASPHONEBOOKDLG nsAutodial::mpRasPhonebookDlg = nsnull;
tRASENUMCONNECTIONS nsAutodial::mpRasEnumConnections = nsnull;
tRASENUMENTRIES nsAutodial::mpRasEnumEntries = nsnull;
tRASDIALDLG nsAutodial::mpRasDialDlg = nsnull;
tRASSETAUTODIALADDRESS nsAutodial::mpRasSetAutodialAddress = nsnull;
tRASGETAUTODIALADDRESS nsAutodial::mpRasGetAutodialAddress = nsnull;
tRASGETAUTODIALENABLE nsAutodial::mpRasGetAutodialEnable = nsnull;
tRASGETAUTODIALPARAM nsAutodial::mpRasGetAutodialParam = nsnull;

HINSTANCE nsAutodial::mhRASdlg = nsnull;
HINSTANCE nsAutodial::mhRASapi32 = nsnull;


nsAutodial::nsAutodial()
:   mAutodialBehavior(AUTODIAL_DEFAULT),
    mNumRASConnectionEntries(0),
    mAutodialServiceDialingLocation(-1)
{
    mOSVerInfo.dwOSVersionInfoSize = sizeof(mOSVerInfo);
    GetVersionEx(&mOSVerInfo);

    
    
    Init();
}


nsAutodial::~nsAutodial()
{
}





nsresult nsAutodial::Init()
{
#ifdef PR_LOGGING
    if (!gLog)
        gLog = PR_NewLogModule("Autodial");
#endif

    mDefaultEntryName[0] = '\0';
    mNumRASConnectionEntries = 0;
    mAutodialBehavior = QueryAutodialBehavior();
    
    
    if (mAutodialBehavior == AUTODIAL_NEVER)
    {
        return NS_OK;
    }


    
    mNumRASConnectionEntries = NumRASEntries();
    
    
    nsresult result = GetDefaultEntryName(mDefaultEntryName,
                                          sizeof(mDefaultEntryName));
    
    return result;
}





PRBool nsAutodial::ShouldDialOnNetworkError()
{
    
    if (mDontRetryUntil) 
    {
        PRIntervalTime intervalNow = PR_IntervalNow();
        if (intervalNow < mDontRetryUntil) 
        {
            LOGD(("Autodial: Not dialing: too soon."));
            return PR_FALSE;
        }
    }
     

    return ((mAutodialBehavior == AUTODIAL_ALWAYS) 
             || (mAutodialBehavior == AUTODIAL_ON_NETWORKERROR)
             || (mAutodialBehavior == AUTODIAL_USE_SERVICE));
}






int nsAutodial::QueryAutodialBehavior()
{
    if (IsAutodialServiceRunning())
    {
        if (!LoadRASapi32DLL())
            return AUTODIAL_NEVER;

        
        DWORD disabled = 0;
        DWORD size = sizeof(DWORD);
        if ((*mpRasGetAutodialParam)(RASADP_LoginSessionDisable, &disabled, &size) == ERROR_SUCCESS)
        {
            if (!disabled)
            {
                
                mAutodialServiceDialingLocation = GetCurrentLocation();
                if (IsAutodialServiceEnabled(mAutodialServiceDialingLocation))
                {
                    return AUTODIAL_USE_SERVICE;
                }
            }
        }
    }

    
    
    HKEY hKey = 0;
    LONG result = ::RegOpenKeyExW(
                    HKEY_CURRENT_USER, 
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 
                    0, 
                    KEY_READ, 
                    &hKey);

    if (result != ERROR_SUCCESS)
    {
        LOGE(("Autodial: Error opening reg key Internet Settings"));
        return AUTODIAL_NEVER;
    }

    DWORD entryType = 0;
    DWORD autodial = 0;
    DWORD onDemand = 0;
    DWORD paramSize = sizeof(DWORD);

    result = ::RegQueryValueExW(hKey, L"EnableAutodial", nsnull, &entryType, (LPBYTE)&autodial, &paramSize);
    if (result != ERROR_SUCCESS)
    {
        ::RegCloseKey(hKey);
        LOGE(("Autodial: Error reading reg value EnableAutodial."));
        return AUTODIAL_NEVER;
    }

    result = ::RegQueryValueExW(hKey, L"NoNetAutodial", nsnull, &entryType, (LPBYTE)&onDemand, &paramSize);
    if (result != ERROR_SUCCESS)
    {
        ::RegCloseKey(hKey);
        LOGE(("Autodial: Error reading reg value NoNetAutodial."));
        return AUTODIAL_NEVER;
    }
  
    ::RegCloseKey(hKey);

    if (!autodial)
    {
        return AUTODIAL_NEVER;
    }
    else 
    {
        if (onDemand)
        {
            return AUTODIAL_ON_NETWORKERROR;
        }
        else
        {
            return AUTODIAL_ALWAYS;
        }
    }
}













nsresult nsAutodial::DialDefault(const PRUnichar* hostName)
{
    mDontRetryUntil = 0;

    if (mAutodialBehavior == AUTODIAL_NEVER)
    {
        return NS_ERROR_FAILURE;    
    }

    
    if (IsRASConnected())
    {
        LOGD(("Autodial: Not dialing: active connection."));
        return NS_ERROR_FAILURE;    
    }

    
    if (mNumRASConnectionEntries <= 0)
    {
        LOGD(("Autodial: Not dialing: no entries."));
        return NS_ERROR_FAILURE;    
    }


    
    
    
    
    
    
    
    
    if (mAutodialBehavior == AUTODIAL_USE_SERVICE)
    {
        AddAddressToAutodialDirectory(hostName);
        return NS_ERROR_FAILURE;    
    }

    
    else
    {
        
        if (!LoadRASdlgDLL())
            return NS_ERROR_NULL_POINTER;

        
        if (mDefaultEntryName[0] != '\0') 
        {
            LOGD(("Autodial: Dialing default: %s.",mDefaultEntryName));

            RASDIALDLG rasDialDlg;
            memset(&rasDialDlg, 0, sizeof(rasDialDlg));
            rasDialDlg.dwSize = sizeof(rasDialDlg);

            PRBool dialed = 
             (*mpRasDialDlg)(nsnull, mDefaultEntryName, nsnull, &rasDialDlg);

            if (!dialed)
            {
                if (rasDialDlg.dwError != 0)
                {
                    LOGE(("Autodial ::RasDialDlg failed: Error: %d.", 
                     rasDialDlg.dwError));
                }
                else
                {
                    mDontRetryUntil = PR_IntervalNow() + PR_SecondsToInterval(NO_RETRY_PERIOD_SEC);
                    LOGD(("Autodial: User cancelled dial."));
                }
                return NS_ERROR_FAILURE;    
            }

            LOGD(("Autodial: RAS dialup connection successful."));
        }

        
        
        else
        {
            LOGD(("Autodial: Prompting for phonebook entry."));

            RASPBDLG rasPBDlg;
            memset(&rasPBDlg, 0, sizeof(rasPBDlg));
            rasPBDlg.dwSize = sizeof(rasPBDlg);
 
            PRBool dialed = (*mpRasPhonebookDlg)(nsnull, nsnull, &rasPBDlg);

            if (!dialed)
            {
                if (rasPBDlg.dwError != 0)
                {
                    LOGE(("Autodial: ::RasPhonebookDlg failed: Error = %d.", 
                     rasPBDlg.dwError));
                }
                else
                {
                    mDontRetryUntil = PR_IntervalNow() + PR_SecondsToInterval(NO_RETRY_PERIOD_SEC);
                    LOGD(("Autodial: User cancelled dial."));
                }

                return NS_ERROR_FAILURE;    
            }

            LOGD(("Autodial: RAS dialup connection successful."));
        }
    }

    
    return NS_OK;
}



PRBool nsAutodial::IsRASConnected()
{
    DWORD connections;
    RASCONN rasConn;
    rasConn.dwSize = sizeof(rasConn);
    DWORD structSize = sizeof(rasConn);

    if (!LoadRASapi32DLL())
        return NS_ERROR_NULL_POINTER;

    DWORD result = (*mpRasEnumConnections)(&rasConn, &structSize, &connections);

    
    if (result == ERROR_SUCCESS || result == ERROR_BUFFER_TOO_SMALL)
    {
        return (connections > 0);
    }

    LOGE(("Autodial: ::RasEnumConnections failed: Error = %d", result));
    return PR_FALSE;
}


nsresult nsAutodial::GetFirstEntryName(PRUnichar* entryName, int bufferSize)
{
    
    if (!LoadRASapi32DLL())
        return NS_ERROR_NULL_POINTER;

    RASENTRYNAMEW rasEntryName;
    rasEntryName.dwSize = sizeof(rasEntryName);
    DWORD cb = sizeof(rasEntryName);
    DWORD cEntries = 0;

    DWORD result = 
     (*mpRasEnumEntries)(nsnull, nsnull, &rasEntryName, &cb, &cEntries);

    
    if (result == ERROR_SUCCESS || result == ERROR_BUFFER_TOO_SMALL)
    {
        wcsncpy(entryName, rasEntryName.szEntryName,
                bufferSize / sizeof(*entryName));
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}


int nsAutodial::NumRASEntries()
{
    
    if (!LoadRASapi32DLL())
        return 0;

    RASENTRYNAMEW rasEntryName;
    rasEntryName.dwSize = sizeof(rasEntryName);
    DWORD cb = sizeof(rasEntryName);
    DWORD cEntries = 0;


    DWORD result = 
     (*mpRasEnumEntries)(nsnull, nsnull, &rasEntryName, &cb, &cEntries);

    
    if (result == ERROR_SUCCESS || result == ERROR_BUFFER_TOO_SMALL)
    {
        return (int)cEntries;
    }

    return 0;
}


nsresult nsAutodial::GetDefaultEntryName(PRUnichar* entryName, int bufferSize)
{
    
    if (mNumRASConnectionEntries <= 0)
    {
        return NS_ERROR_FAILURE;
    }

    
    if (mNumRASConnectionEntries == 1)
    {
        return GetFirstEntryName(entryName, bufferSize);
    }

    
    
    
    
    
    

    const PRUnichar* key = nsnull;
    const PRUnichar* val = nsnull;

    HKEY hKey = 0;
    LONG result = 0;

    
    if ((mOSVerInfo.dwMajorVersion == 4) 
     || ((mOSVerInfo.dwMajorVersion == 5) && (mOSVerInfo.dwMinorVersion == 0))) 
    {
        key = L"RemoteAccess";
        val = L"InternetProfile";

        result = ::RegOpenKeyExW(
                    HKEY_CURRENT_USER, 
                    key, 
                    0, 
                    KEY_READ, 
                    &hKey);

        if (result != ERROR_SUCCESS)
        {
            return NS_ERROR_FAILURE;
        }
    }
    else  
    {
        key = L"Software\\Microsoft\\RAS Autodial\\Default";
        val = L"DefaultInternet";

        
        
        result = ::RegOpenKeyExW(
                    HKEY_CURRENT_USER, 
                    key, 
                    0, 
                    KEY_READ, 
                    &hKey);

        if (result != ERROR_SUCCESS)
        {
            
            result = ::RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE, 
                        key, 
                        0, 
                        KEY_READ, 
                        &hKey);

            if (result != ERROR_SUCCESS)
            {
                return NS_ERROR_FAILURE;
            }
        }
    }


    DWORD entryType = 0;
    DWORD buffSize = bufferSize;

    result = ::RegQueryValueExW(hKey, 
                                val, 
                                nsnull, 
                                &entryType, 
                                (LPBYTE)entryName, 
                                &buffSize);

    ::RegCloseKey(hKey);


    if (result != ERROR_SUCCESS)
    {
        
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}



PRBool nsAutodial::IsAutodialServiceRunning()
{
    SC_HANDLE hSCManager = 
      OpenSCManager(nsnull, SERVICES_ACTIVE_DATABASE, SERVICE_QUERY_STATUS);

    if (hSCManager == nsnull)
    {
        LOGE(("Autodial: failed to open service control manager. Error %d.", 
          ::GetLastError()));

        return PR_FALSE;
    }

    SC_HANDLE hService = 
      OpenServiceW(hSCManager, L"RasAuto", SERVICE_QUERY_STATUS);

    if (hSCManager == nsnull)
    {
        LOGE(("Autodial: failed to open RasAuto service."));
        return PR_FALSE;
    }

    SERVICE_STATUS status;
    if (!QueryServiceStatus(hService, &status))
    {
        LOGE(("Autodial: ::QueryServiceStatus() failed. Error: %d", 
          ::GetLastError()));

        return PR_FALSE;
    }

    return (status.dwCurrentState == SERVICE_RUNNING);
}


PRBool nsAutodial::AddAddressToAutodialDirectory(const PRUnichar* hostName)
{
    
    if (!LoadRASapi32DLL())
        return PR_FALSE;

    
    RASAUTODIALENTRYW autodialEntry;
    autodialEntry.dwSize = sizeof(autodialEntry);
    DWORD size = sizeof(autodialEntry);
    DWORD entries = 0;

    DWORD result = (*mpRasGetAutodialAddress)(hostName, 
                                              nsnull, 
                                              &autodialEntry, 
                                              &size, 
                                              &entries);

    
    if (result != ERROR_FILE_NOT_FOUND)
    {
        LOGD(("Autodial: Address %s already in autodial db.", hostName));
        return PR_FALSE;
    }

    autodialEntry.dwSize = sizeof(autodialEntry);
    autodialEntry.dwFlags = 0;
    autodialEntry.dwDialingLocation = mAutodialServiceDialingLocation;
    GetDefaultEntryName(autodialEntry.szEntry, sizeof(autodialEntry.szEntry));

    result = (*mpRasSetAutodialAddress)(hostName, 
                                        0, 
                                        &autodialEntry, 
                                        sizeof(autodialEntry), 
                                        1);

    if (result != ERROR_SUCCESS)
    {
        LOGE(("Autodial ::RasSetAutodialAddress failed result %d.", result));
        return PR_FALSE;
    }

    LOGD(("Autodial: Added address %s to RAS autodial db for entry %s.",
         hostName, NS_ConvertUTF16toUTF8(autodialEntry.szEntry).get()));

    return PR_TRUE;
}


int nsAutodial::GetCurrentLocation()
{
    HKEY hKey = 0;
    LONG result = ::RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE, 
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations", 
                    0, 
                    KEY_READ, 
                    &hKey);

    if (result != ERROR_SUCCESS)
    {
        LOGE(("Autodial: Error opening reg key ...CurrentVersion\\Telephony\\Locations"));
        return -1;
    }

    DWORD entryType = 0;
    DWORD location = 0;
    DWORD paramSize = sizeof(DWORD);

    result = ::RegQueryValueExW(hKey, L"CurrentID", nsnull, &entryType, (LPBYTE)&location, &paramSize);
    if (result != ERROR_SUCCESS)
    {
        ::RegCloseKey(hKey);
        LOGE(("Autodial: Error reading reg value CurrentID."));
        return -1;
    }

    ::RegCloseKey(hKey);
    return location;

}


PRBool nsAutodial::IsAutodialServiceEnabled(int location)
{
    if (location < 0)
        return PR_FALSE;

    if (!LoadRASapi32DLL())
        return PR_FALSE;

    PRBool enabled;
    if ((*mpRasGetAutodialEnable)(location, &enabled) != ERROR_SUCCESS)
    {
        LOGE(("Autodial: Error calling RasGetAutodialEnable()"));
        return PR_FALSE;
    }

    return enabled;
}



PRBool nsAutodial::LoadRASapi32DLL()
{
    if (!mhRASapi32)
    {
        mhRASapi32 = ::LoadLibraryW(L"rasapi32.dll");
        if ((UINT_PTR)mhRASapi32 > 32)
        {
            
            mpRasEnumConnections = (tRASENUMCONNECTIONS)
             ::GetProcAddress(mhRASapi32, "RasEnumConnectionsW");

            
            mpRasEnumEntries = (tRASENUMENTRIES)
             ::GetProcAddress(mhRASapi32, "RasEnumEntriesW");

            
            mpRasSetAutodialAddress = (tRASSETAUTODIALADDRESS)
                ::GetProcAddress(mhRASapi32, "RasSetAutodialAddressW");

            
            mpRasGetAutodialAddress = (tRASGETAUTODIALADDRESS)
             ::GetProcAddress(mhRASapi32, "RasGetAutodialAddressW");

            
            mpRasGetAutodialEnable = (tRASGETAUTODIALENABLE)
             ::GetProcAddress(mhRASapi32, "RasGetAutodialEnableW");

            
            mpRasGetAutodialParam = (tRASGETAUTODIALPARAM)
             ::GetProcAddress(mhRASapi32, "RasGetAutodialParamW");
        }

    }

    if (!mhRASapi32 
        || !mpRasEnumConnections 
        || !mpRasEnumEntries 
        || !mpRasSetAutodialAddress
        || !mpRasGetAutodialAddress
        || !mpRasGetAutodialEnable
        || !mpRasGetAutodialParam)
    {
        LOGE(("Autodial: Error loading RASAPI32.DLL."));
        return PR_FALSE;
    }

    return PR_TRUE;
}

PRBool nsAutodial::LoadRASdlgDLL()
{
    if (!mhRASdlg)
    {
        mhRASdlg = ::LoadLibraryW(L"rasdlg.dll");
        if ((UINT_PTR)mhRASdlg > 32)
        {
            
            mpRasPhonebookDlg =
             (tRASPHONEBOOKDLG)::GetProcAddress(mhRASdlg, "RasPhonebookDlgW");

            
            mpRasDialDlg =
             (tRASDIALDLG)::GetProcAddress(mhRASdlg, "RasDialDlgW");

        }
    }

    if (!mhRASdlg || !mpRasPhonebookDlg || !mpRasDialDlg)
    {
        LOGE(("Autodial: Error loading RASDLG.DLL."));
        return PR_FALSE;
    }

    return PR_TRUE;
}

