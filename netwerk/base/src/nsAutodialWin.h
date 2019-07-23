




































#ifndef nsAutodialWin_h__
#define nsAutodialWin_h__

#include <windows.h>
#include <ras.h>
#include <rasdlg.h>
#include <raserror.h>
#include "nscore.h"
#include "nspr.h"

#if (WINVER < 0x401)


typedef struct tagRASAUTODIALENTRYA {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwDialingLocation;
    PRUnichar szEntry[RAS_MaxEntryName + 1];
} RASAUTODIALENTRYW, *LPRASAUTODIALENTRYW;
typedef RASAUTODIALENTRYW RASAUTODIALENTRY, *LPRASAUTODIALENTRY;

#define RASADP_LoginSessionDisable              1

#endif  


typedef DWORD (WINAPI* tRASPHONEBOOKDLG)(LPWSTR,LPWSTR,LPRASPBDLG);
typedef DWORD (WINAPI* tRASDIALDLG)(LPWSTR,LPWSTR,LPWSTR,LPRASDIALDLG);
typedef DWORD (WINAPI* tRASENUMCONNECTIONS)(LPRASCONN,LPDWORD,LPDWORD);
typedef DWORD (WINAPI* tRASENUMENTRIES)(LPWSTR,LPWSTR,LPRASENTRYNAMEW,LPDWORD,LPDWORD);
typedef DWORD (WINAPI* tRASSETAUTODIALADDRESS)(LPCWSTR,DWORD,LPRASAUTODIALENTRYW,DWORD,DWORD);
typedef DWORD (WINAPI* tRASGETAUTODIALADDRESS)(LPCWSTR,LPDWORD,LPRASAUTODIALENTRYW,LPDWORD,LPDWORD);
typedef DWORD (WINAPI* tRASGETAUTODIALENABLE)(DWORD,LPBOOL);
typedef DWORD (WINAPI* tRASGETAUTODIALPARAM)(DWORD,LPVOID,LPDWORD);























class nsAutodial
{
private:

    
    
    

    
    PRBool IsAutodialServiceRunning();

    
    int NumRASEntries();

    
    nsresult GetDefaultEntryName(PRUnichar* entryName, int bufferSize);

    
    nsresult GetFirstEntryName(PRUnichar* entryName, int bufferSize);

    
    PRBool IsRASConnected();

    
    int QueryAutodialBehavior();

    
    PRBool AddAddressToAutodialDirectory(const PRUnichar* hostName);

    
    int GetCurrentLocation();

    
    PRBool IsAutodialServiceEnabled(int location);

    
    
    
    
    
    int mAutodialBehavior;

    int mAutodialServiceDialingLocation;

    enum { AUTODIAL_NEVER = 1 };            
    enum { AUTODIAL_ALWAYS = 2 };           
    enum { AUTODIAL_ON_NETWORKERROR = 3 };  
    enum { AUTODIAL_USE_SERVICE = 4 };      

    
    int mNumRASConnectionEntries;

    
    PRUnichar mDefaultEntryName[RAS_MaxEntryName + 1];  

    
    static PRIntervalTime mDontRetryUntil;

    
    OSVERSIONINFO mOSVerInfo;

    
    static HINSTANCE mhRASdlg;
    static HINSTANCE mhRASapi32;

    
    static tRASPHONEBOOKDLG mpRasPhonebookDlg;
    static tRASENUMCONNECTIONS	mpRasEnumConnections;
    static tRASENUMENTRIES mpRasEnumEntries;
    static tRASDIALDLG mpRasDialDlg;
    static tRASSETAUTODIALADDRESS mpRasSetAutodialAddress;
    static tRASGETAUTODIALADDRESS mpRasGetAutodialAddress;
    static tRASGETAUTODIALENABLE mpRasGetAutodialEnable;
    static tRASGETAUTODIALPARAM mpRasGetAutodialParam;

    PRBool LoadRASapi32DLL();
    PRBool LoadRASdlgDLL();


public:
  
    
    nsAutodial();

    
    virtual ~nsAutodial();

    
    
    nsresult Init();

    
    nsresult DialDefault(const PRUnichar* hostName);

    
    PRBool ShouldDialOnNetworkError();
};

#endif 

