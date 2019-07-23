






































#include "nsLocalHandlerApp.h"



NS_IMPL_ISUPPORTS2(nsLocalHandlerApp, nsILocalHandlerApp, nsIHandlerApp)




NS_IMETHODIMP nsLocalHandlerApp::GetName(nsAString& aName)
{
  if (mName.IsEmpty() && mExecutable) {
    
    
    mExecutable->GetLeafName(aName);
  } else {
    aName.Assign(mName);
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsLocalHandlerApp::SetName(const nsAString & aName)
{
  mName.Assign(aName);

  return NS_OK;
}




NS_IMETHODIMP nsLocalHandlerApp::GetExecutable(nsIFile **aExecutable)
{
  NS_IF_ADDREF(*aExecutable = mExecutable);
  return NS_OK;
}

NS_IMETHODIMP nsLocalHandlerApp::SetExecutable(nsIFile *aExecutable)
{
  mExecutable = aExecutable;
  return NS_OK;
}

