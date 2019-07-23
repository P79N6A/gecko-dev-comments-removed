




































#include "nsNativeAppSupport.h"

#include "nsCOMPtr.h"












class nsNativeAppSupportBase : public nsINativeAppSupport {
public:
    nsNativeAppSupportBase();
    virtual ~nsNativeAppSupportBase();

    
    NS_DECL_NSINATIVEAPPSUPPORT

    NS_IMETHOD CreateSplashScreen( nsISplashScreen **splash );

    
    NS_IMETHOD_(nsrefcnt) AddRef();
    NS_IMETHOD_(nsrefcnt) Release();
    NS_IMETHOD QueryInterface( const nsIID &iid, void**p );

    nsrefcnt mRefCnt;
    nsCOMPtr<nsISplashScreen> mSplash;
    PRBool   mServerMode;
    PRBool   mShouldShowUI;
    PRBool   mShownTurboDialog;
    static PRBool   mLastWindowIsConfirmation;

}; 

