




































#ifndef nsAutodialWin_h__
#define nsAutodialWin_h__

#include <windows.h>
#include <ras.h>
#include <rasdlg.h>
#include <raserror.h>
#include "nscore.h"
#include "nspr.h"
























class nsAutodial
{
private:

    
    
    

    
    bool IsAutodialServiceRunning();

    
    int NumRASEntries();

    
    nsresult GetDefaultEntryName(PRUnichar* entryName, int bufferSize);

    
    nsresult GetFirstEntryName(PRUnichar* entryName, int bufferSize);

    
    bool IsRASConnected();

    
    int QueryAutodialBehavior();

    
    bool AddAddressToAutodialDirectory(const PRUnichar* hostName);

    
    int GetCurrentLocation();

    
    bool IsAutodialServiceEnabled(int location);

    
    
    
    
    
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

public:
  
    
    nsAutodial();

    
    virtual ~nsAutodial();

    
    
    nsresult Init();

    
    nsresult DialDefault(const PRUnichar* hostName);

    
    bool ShouldDialOnNetworkError();
};

#endif 

