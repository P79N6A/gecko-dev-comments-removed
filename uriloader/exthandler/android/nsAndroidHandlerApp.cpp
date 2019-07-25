




































#include "nsAndroidHandlerApp.h"
#include "AndroidBridge.h"


NS_IMPL_ISUPPORTS1(nsAndroidHandlerApp, nsIHandlerApp)


nsAndroidHandlerApp::nsAndroidHandlerApp(const nsAString& aName,
                                         const nsAString& aDescription,
                                         const nsAString& aPackageName,
                                         const nsAString& aClassName,
                                         const nsACString& aMimeType) :
mName(aName), mDescription(aDescription), mPackageName(aPackageName),
  mClassName(aClassName), mMimeType(aMimeType)
{
}

nsAndroidHandlerApp::~nsAndroidHandlerApp()
{
}

nsresult nsAndroidHandlerApp::GetName(nsAString & aName)
{
  aName.Assign(mName);
  return NS_OK;
}

nsresult nsAndroidHandlerApp::SetName(const nsAString & aName)
{
  mName.Assign(aName);
  return NS_OK;
}

nsresult nsAndroidHandlerApp::GetDetailedDescription(nsAString & aDescription)
{
  aDescription.Assign(mDescription);
  return NS_OK;
}

nsresult nsAndroidHandlerApp::SetDetailedDescription(const nsAString & aDescription)
{
  mDescription.Assign(aDescription);

  return NS_OK;
}

nsresult nsAndroidHandlerApp::Equals(nsIHandlerApp *aHandlerApp, PRBool *aRetval)
{
  nsCOMPtr<nsAndroidHandlerApp> aApp = do_QueryInterface(aHandlerApp);
  *aRetval = aApp && aApp->mName.Equals(mName) &&
    aApp->mDescription.Equals(mDescription);
  return NS_OK;
}

nsresult nsAndroidHandlerApp::LaunchWithURI(nsIURI *aURI, nsIInterfaceRequestor *aWindowContext)
{
  nsCString uriSpec;
  aURI->GetSpec(uriSpec);
  return mozilla::AndroidBridge::Bridge()->
    OpenUriExternal(uriSpec, mMimeType, mPackageName, mClassName) ? 
    NS_OK : NS_ERROR_FAILURE;

}
