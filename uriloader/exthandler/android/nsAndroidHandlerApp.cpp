




































#include "nsAndroidHandlerApp.h"

NS_IMPL_ISUPPORTS1(nsAndroidHandlerApp, nsIHandlerApp)


nsAndroidHandlerApp::nsAndroidHandlerApp(nsAString& aName,
                                         nsAString& aDescription) :
mName(aName), mDescription(aDescription)
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
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
