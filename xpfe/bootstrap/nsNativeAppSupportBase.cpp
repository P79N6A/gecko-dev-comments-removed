




































#include "nsNativeAppSupportBase.h"

PRBool nsNativeAppSupportBase::mLastWindowIsConfirmation = PR_FALSE;

nsNativeAppSupportBase::nsNativeAppSupportBase()
    : mRefCnt( 0 ),
      mSplash( 0 ),
      mServerMode( PR_FALSE ),
      mShouldShowUI( PR_TRUE ),
      mShownTurboDialog( PR_FALSE ) {
}

nsNativeAppSupportBase::~nsNativeAppSupportBase() {
}


NS_IMETHODIMP
nsNativeAppSupportBase::Start( PRBool *result ) {
    NS_ENSURE_ARG( result );
    *result = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportBase::Stop( PRBool *result ) {
    NS_ENSURE_ARG( result );
    *result = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportBase::Quit() {
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::ReOpen()
{
  return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportBase::ShowSplashScreen() {
    if ( !mSplash ) {
        CreateSplashScreen( getter_AddRefs( mSplash ) );
    }
    if ( mSplash ) {
        mSplash->Show();
    }
    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportBase::HideSplashScreen() {
    
    if ( mSplash ) {
        
        nsCOMPtr<nsISplashScreen> splash = mSplash;
        mSplash = 0;
        
        splash->Hide();
    }
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::SetIsServerMode(PRBool aIsServerMode) {
    mServerMode = aIsServerMode;
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::GetIsServerMode(PRBool *aIsServerMode) {
    NS_ENSURE_ARG( aIsServerMode );
    *aIsServerMode = mServerMode;
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::SetShouldShowUI(PRBool aShouldShowUI) {
    mShouldShowUI = aShouldShowUI;
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::GetShouldShowUI(PRBool *aShouldShowUI) {
    NS_ENSURE_ARG( aShouldShowUI );
    *aShouldShowUI = mShouldShowUI;
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::StartServerMode() {
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::OnLastWindowClosing() {
    return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportBase::CreateSplashScreen( nsISplashScreen **splash ) {
    NS_ENSURE_ARG( splash );
    *splash = 0;
    return NS_CreateSplashScreen( splash );
}


NS_IMETHODIMP_(nsrefcnt)
nsNativeAppSupportBase::AddRef() {
    ++mRefCnt;
    return mRefCnt;
}

NS_IMETHODIMP_(nsrefcnt)
nsNativeAppSupportBase::Release() {
    --mRefCnt;
    if ( !mRefCnt ) {
        delete this;
        return 0;
    }
    return mRefCnt;
}

NS_IMETHODIMP
nsNativeAppSupportBase::QueryInterface( const nsIID &iid, void**p ) {
    nsresult rv = NS_OK;
    if ( p ) {
        *p = 0;
        if ( iid.Equals( NS_GET_IID( nsINativeAppSupport ) ) ) {
            nsINativeAppSupport *result = this;
            *p = result;
            NS_ADDREF( result );
        } else if ( iid.Equals( NS_GET_IID( nsISupports ) ) ) {
            nsISupports *result = NS_STATIC_CAST( nsISupports*, this );
            *p = result;
            NS_ADDREF( result );
        } else {
            rv = NS_NOINTERFACE;
        }
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

NS_IMETHODIMP
nsNativeAppSupportBase::EnsureProfile(nsICmdLineService* args)
{
    return NS_OK;
}
