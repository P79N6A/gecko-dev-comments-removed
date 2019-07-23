#include "nsHandlerAppImpl.h"



NS_IMPL_ISUPPORTS1(nsHandlerAppBase, nsIHandlerApp)


NS_IMETHODIMP nsHandlerAppBase::GetName(nsAString & aName)
{
  aName.Assign(mName);
  
  return NS_OK;
}

NS_IMETHODIMP nsHandlerAppBase::SetName(const nsAString & aName)
{
  mName.Assign(aName);

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED1(nsLocalHandlerApp, nsHandlerAppBase, nsILocalHandlerApp)



NS_IMETHODIMP nsLocalHandlerApp::GetName(nsAString& aName)
{
  if (mName.IsEmpty() && mExecutable) {
    
    
    mExecutable->GetLeafName(aName);
  } else {
    aName = mName;
  }
  
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


NS_IMPL_ISUPPORTS_INHERITED1(nsWebHandlerApp, nsHandlerAppBase,
                             nsIWebHandlerApp)


NS_IMETHODIMP nsWebHandlerApp::GetUriTemplate(nsACString &aUriTemplate)
{
  aUriTemplate.Assign(mUriTemplate);
  return NS_OK;
}

NS_IMETHODIMP nsWebHandlerApp::SetUriTemplate(const nsACString &aUriTemplate)
{
  mUriTemplate.Assign(aUriTemplate);
  return NS_OK;
}