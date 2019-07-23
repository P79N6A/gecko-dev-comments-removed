




































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
    CHAR szEntry[RAS_MaxEntryName + 1];
} RASAUTODIALENTRYA, *LPRASAUTODIALENTRYA;
typedef RASAUTODIALENTRYA RASAUTODIALENTRY, *LPRASAUTODIALENTRY;

#define RASADP_LoginSessionDisable              1

#endif  


typedef DWORD (WINAPI* tRASPHONEBOOKDLG)(LPTSTR,LPTSTR,LPRASPBDLG);
typedef DWORD (WINAPI* tRASDIALDLG)(LPTSTR,LPTSTR,LPTSTR,LPRASDIALDLG);
typedef DWORD (WINAPI* tRASENUMCONNECTIONS)(LPRASCONN,LPDWORD,LPDWORD);
typedef DWORD (WINAPI* tRASENUMENTRIES)(LPTSTR,LPTSTR,LPRASENTRYNAME,LPDWORD,LPDWORD);
typedef DWORD (WINAPI* tRASSETAUTODIALADDRESS)(LPCTSTR,DWORD,LPRASAUTODIALENTRY,DWORD,DWORD);
typedef DWORD (WINAPI* tRASGETAUTODIALADDRESS)(LPCTSTR,LPDWORD,LPRASAUTODIALENTRY,LPDWORD,LPDWORD);
typedef DWORD (WINAPI* tRASGETAUTODIALENABLE)(DWORD,LPBOOL);
typedef DWORD (WINAPI* tRASGETAUTODIALPARAM)(DWORD,LPVOID,LPDWORD);























class nsRASAutodial
{
private:

    
    
    

    
    PRBool IsAutodialServiceRunning();

    
    int NumRASEntries();

    
    nsresult GetDefaultEntryName(char* entryName, int bufferSize);

    
    nsresult GetFirstEntryName(char* entryName, int bufferSize);

    
    PRBool IsRASConnected();

    
    int QueryAutodialBehavior();

    
    PRBool AddAddressToAutodialDirectory(const char* hostName);

    
    int GetCurrentLocation();

    
    PRBool IsAutodialServiceEnabled(int location);

    
    
    
    
    
    int mAutodialBehavior;

    int mAutodialServiceDialingLocation;

    enum { AUTODIAL_NEVER = 1 };            
    enum { AUTODIAL_ALWAYS = 2 };           
    enum { AUTODIAL_ON_NETWORKERROR = 3 };  
    enum { AUTODIAL_USE_SERVICE = 4 };      

    
    int mNumRASConnectionEntries;

    
    char mDefaultEntryName[RAS_MaxEntryName + 1];  

    
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
  
    
    nsRASAutodial();

    
    virtual ~nsRASAutodial();

    
    
    nsresult Init();

    
    nsresult DialDefault(const char* hostName);

    
    PRBool ShouldDialOnNetworkError();
};

#endif 

