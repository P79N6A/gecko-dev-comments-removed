










































#include <windows.h>
#include <winsvc.h>
#include "nsAutodialWin.h"
#include "prlog.h"

#ifdef WINCE
#include <objbase.h>
#include <initguid.h>
#include <connmgr.h>
#endif

#ifdef WINCE
#define AUTODIAL_DEFAULT AUTODIAL_ALWAYS
#else
#define AUTODIAL_DEFAULT AUTODIAL_NEVER
#endif













#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = nsnull;
#endif

#define LOGD(args) PR_LOG(gLog, PR_LOG_DEBUG, args)
#define LOGE(args) PR_LOG(gLog, PR_LOG_ERROR, args)


#define NO_RETRY_PERIOD_SEC 5
PRIntervalTime nsRASAutodial::mDontRetryUntil = 0;


tRASPHONEBOOKDLG nsRASAutodial::mpRasPhonebookDlg = nsnull;
tRASENUMCONNECTIONS nsRASAutodial::mpRasEnumConnections = nsnull;
tRASENUMENTRIES nsRASAutodial::mpRasEnumEntries = nsnull;
tRASDIALDLG nsRASAutodial::mpRasDialDlg = nsnull;
tRASSETAUTODIALADDRESS nsRASAutodial::mpRasSetAutodialAddress = nsnull;
tRASGETAUTODIALADDRESS nsRASAutodial::mpRasGetAutodialAddress = nsnull;
tRASGETAUTODIALENABLE nsRASAutodial::mpRasGetAutodialEnable = nsnull;
tRASGETAUTODIALPARAM nsRASAutodial::mpRasGetAutodialParam = nsnull;

HINSTANCE nsRASAutodial::mhRASdlg = nsnull;
HINSTANCE nsRASAutodial::mhRASapi32 = nsnull;


nsRASAutodial::nsRASAutodial()
:   mAutodialBehavior(AUTODIAL_DEFAULT),
    mNumRASConnectionEntries(0),
    mAutodialServiceDialingLocation(-1)
{
    mOSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&mOSVerInfo);

    
    
    Init();
}


nsRASAutodial::~nsRASAutodial()
{
}





nsresult nsRASAutodial::Init()
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
                                           RAS_MaxEntryName + 1);
    
    return result;
}





PRBool nsRASAutodial::ShouldDialOnNetworkError()
{
#ifndef WINCE
    
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
#else
    return PR_TRUE;
#endif
}






int nsRASAutodial::QueryAutodialBehavior()
{
#ifndef WINCE
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
    LONG result = ::RegOpenKeyEx(
                    HKEY_CURRENT_USER, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 
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

    result = ::RegQueryValueEx(hKey, "EnableAutodial", nsnull, &entryType, (LPBYTE)&autodial, &paramSize);
    if (result != ERROR_SUCCESS)
    {
        ::RegCloseKey(hKey);
        LOGE(("Autodial: Error reading reg value EnableAutodial."));
        return AUTODIAL_NEVER;
    }

    result = ::RegQueryValueEx(hKey, "NoNetAutodial", nsnull, &entryType, (LPBYTE)&onDemand, &paramSize);
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
#else
    return AUTODIAL_DEFAULT;
#endif
}


#ifdef WINCE
static nsresult DoPPCConnection()
{
    static HANDLE    gConnectionHandle = NULL;

    
    CONNMGR_CONNECTIONINFO conn_info;
    memset(&conn_info, 0, sizeof(CONNMGR_CONNECTIONINFO));

    conn_info.cbSize      = sizeof(CONNMGR_CONNECTIONINFO);
    conn_info.dwParams    = CONNMGR_PARAM_GUIDDESTNET;
    conn_info.dwPriority  = CONNMGR_PRIORITY_USERINTERACTIVE;
    conn_info.guidDestNet = IID_DestNetInternet;
    conn_info.bExclusive  = FALSE;
    conn_info.bDisabled   = FALSE;

    HANDLE tempConnectionHandle;
    DWORD status;
    HRESULT result = ConnMgrEstablishConnectionSync(&conn_info, 
                                                    &tempConnectionHandle, 
                                                    60000,
                                                    &status);

    if (result != S_OK)
    {
      return NS_ERROR_FAILURE;
    }

    if (status != CONNMGR_STATUS_CONNECTED)
    {
      
      
      ConnMgrReleaseConnection(tempConnectionHandle, 0);
      return NS_ERROR_FAILURE;
    }

    
    
    if (gConnectionHandle)
      ConnMgrReleaseConnection(gConnectionHandle, 0);
      
    gConnectionHandle = tempConnectionHandle;
    return NS_OK;
}

#endif













nsresult nsRASAutodial::DialDefault(const char* hostName)
{
#ifndef WINCE
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
            memset(&rasDialDlg, 0, sizeof(RASDIALDLG));
            rasDialDlg.dwSize = sizeof(RASDIALDLG);

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
            memset(&rasPBDlg, 0, sizeof(RASPBDLG));
            rasPBDlg.dwSize = sizeof(RASPBDLG);
 
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

#else
    return  DoPPCConnection();
#endif
}



PRBool nsRASAutodial::IsRASConnected()
{
    DWORD connections;
    RASCONN rasConn;
    rasConn.dwSize = sizeof(RASCONN);
    DWORD structSize = sizeof(RASCONN);

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


nsresult nsRASAutodial::GetFirstEntryName(char* entryName, int bufferSize)
{
    
    if (!LoadRASapi32DLL())
        return NS_ERROR_NULL_POINTER;

    RASENTRYNAME rasEntryName;
    rasEntryName.dwSize = sizeof(RASENTRYNAME);
    DWORD cb = sizeof(RASENTRYNAME);
    DWORD cEntries = 0;

    DWORD result = 
     (*mpRasEnumEntries)(nsnull, nsnull, &rasEntryName, &cb, &cEntries);

    
    if (result == ERROR_SUCCESS || result == ERROR_BUFFER_TOO_SMALL)
    {
#ifndef WINCE
        strncpy(entryName, rasEntryName.szEntryName, bufferSize);
#endif
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}


int nsRASAutodial::NumRASEntries()
{
    
    if (!LoadRASapi32DLL())
        return 0;

    RASENTRYNAME rasEntryName;
    rasEntryName.dwSize = sizeof(RASENTRYNAME);
    DWORD cb = sizeof(RASENTRYNAME);
    DWORD cEntries = 0;


    DWORD result = 
     (*mpRasEnumEntries)(nsnull, nsnull, &rasEntryName, &cb, &cEntries);

    
    if (result == ERROR_SUCCESS || result == ERROR_BUFFER_TOO_SMALL)
    {
        return (int)cEntries;
    }

    return 0;
}


nsresult nsRASAutodial::GetDefaultEntryName(char* entryName, int bufferSize)
{
    
    if (mNumRASConnectionEntries <= 0)
    {
        return NS_ERROR_FAILURE;
    }

    
    if (mNumRASConnectionEntries == 1)
    {
        return GetFirstEntryName(entryName, bufferSize);
    }

    
    
    
    
    
    

    char* key = nsnull;
    char* val = nsnull;

    HKEY hKey = 0;
    LONG result = 0;

    
    if ((mOSVerInfo.dwMajorVersion == 4) 
     || ((mOSVerInfo.dwMajorVersion == 5) && (mOSVerInfo.dwMinorVersion == 0))) 
    {
        key = "RemoteAccess";
        val = "InternetProfile";

        result = ::RegOpenKeyEx(
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
        key = "Software\\Microsoft\\RAS Autodial\\Default";
        val = "DefaultInternet";

        
        
        result = ::RegOpenKeyEx(
                    HKEY_CURRENT_USER, 
                    key, 
                    0, 
                    KEY_READ, 
                    &hKey);

        if (result != ERROR_SUCCESS)
        {
            
            result = ::RegOpenKeyEx(
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

    result = ::RegQueryValueEx(hKey, 
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



PRBool nsRASAutodial::IsAutodialServiceRunning()
{
#ifndef WINCE
    SC_HANDLE hSCManager = 
      OpenSCManager(nsnull, SERVICES_ACTIVE_DATABASE, SERVICE_QUERY_STATUS);

    if (hSCManager == nsnull)
    {
        LOGE(("Autodial: failed to open service control manager. Error %d.", 
          ::GetLastError()));

        return PR_FALSE;
    }

    SC_HANDLE hService = 
      OpenService(hSCManager, "RasAuto", SERVICE_QUERY_STATUS);

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
#else
    return PR_TRUE;
#endif
}


PRBool nsRASAutodial::AddAddressToAutodialDirectory(const char* hostName)
{
    
    if (!LoadRASapi32DLL())
        return PR_FALSE;

    
    RASAUTODIALENTRY autodialEntry;
    autodialEntry.dwSize = sizeof(RASAUTODIALENTRY);
    DWORD size = sizeof(RASAUTODIALENTRY);
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

    autodialEntry.dwSize = sizeof(RASAUTODIALENTRY);
    autodialEntry.dwFlags = 0;
    autodialEntry.dwDialingLocation = mAutodialServiceDialingLocation;
    GetDefaultEntryName(autodialEntry.szEntry, RAS_MaxEntryName);

    result = (*mpRasSetAutodialAddress)(hostName, 
                                    0, 
                                    &autodialEntry, 
                                    sizeof(RASAUTODIALENTRY), 
                                    1);

    if (result != ERROR_SUCCESS)
    {
        LOGE(("Autodial ::RasSetAutodialAddress failed result %d.", result));
        return PR_FALSE;
    }

    LOGD(("Autodial: Added address %s to RAS autodial db for entry %s.",
     hostName, autodialEntry.szEntry));

    return PR_TRUE;
}


int nsRASAutodial::GetCurrentLocation()
{
    HKEY hKey = 0;
    LONG result = ::RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations", 
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

    result = ::RegQueryValueEx(hKey, "CurrentID", nsnull, &entryType, (LPBYTE)&location, &paramSize);
    if (result != ERROR_SUCCESS)
    {
        ::RegCloseKey(hKey);
        LOGE(("Autodial: Error reading reg value CurrentID."));
        return -1;
    }

    ::RegCloseKey(hKey);
    return location;

}


PRBool nsRASAutodial::IsAutodialServiceEnabled(int location)
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



PRBool nsRASAutodial::LoadRASapi32DLL()
{
    if (!mhRASapi32)
    {
        mhRASapi32 = ::LoadLibrary("rasapi32.dll");
        if ((UINT)mhRASapi32 > 32)
        {
            
            mpRasEnumConnections = (tRASENUMCONNECTIONS)
             ::GetProcAddress(mhRASapi32, "RasEnumConnectionsA");

            
            mpRasEnumEntries = (tRASENUMENTRIES)
             ::GetProcAddress(mhRASapi32, "RasEnumEntriesA");

            
            mpRasSetAutodialAddress = (tRASSETAUTODIALADDRESS)
                ::GetProcAddress(mhRASapi32, "RasSetAutodialAddressA");

            
            mpRasGetAutodialAddress = (tRASGETAUTODIALADDRESS)
             ::GetProcAddress(mhRASapi32, "RasGetAutodialAddressA");

            
            mpRasGetAutodialEnable = (tRASGETAUTODIALENABLE)
             ::GetProcAddress(mhRASapi32, "RasGetAutodialEnableA");

            
            mpRasGetAutodialParam = (tRASGETAUTODIALPARAM)
             ::GetProcAddress(mhRASapi32, "RasGetAutodialParamA");
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

PRBool nsRASAutodial::LoadRASdlgDLL()
{
    if (!mhRASdlg)
    {
        mhRASdlg = ::LoadLibrary("rasdlg.dll");
        if ((UINT)mhRASdlg > 32)
        {
            
            mpRasPhonebookDlg =
             (tRASPHONEBOOKDLG)::GetProcAddress(mhRASdlg, "RasPhonebookDlgA");

            
            mpRasDialDlg =
             (tRASDIALDLG)::GetProcAddress(mhRASdlg, "RasDialDlgA");

        }
    }

    if (!mhRASdlg || !mpRasPhonebookDlg || !mpRasDialDlg)
    {
        LOGE(("Autodial: Error loading RASDLG.DLL."));
        return PR_FALSE;
    }

    return PR_TRUE;
}

