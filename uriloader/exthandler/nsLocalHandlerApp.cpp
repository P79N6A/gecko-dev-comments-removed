







































#include "nsLocalHandlerApp.h"
#include "nsIURI.h"
#include "nsIProcess.h"



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

NS_IMETHODIMP
nsLocalHandlerApp::Equals(nsIHandlerApp *aHandlerApp, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aHandlerApp);

  
  nsCOMPtr <nsILocalHandlerApp> localHandlerApp = do_QueryInterface(aHandlerApp);
  if (!localHandlerApp) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIFile> executable;
  nsresult rv = localHandlerApp->GetExecutable(getter_AddRefs(executable));
  if (NS_FAILED(rv) || !executable || !mExecutable) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  return executable->Equals(mExecutable, _retval);
}

NS_IMETHODIMP
nsLocalHandlerApp::LaunchWithURI(nsIURI *aURI,
                                 nsIInterfaceRequestor *aWindowContext)
{
  
  nsCAutoString spec;
  aURI->GetAsciiSpec(spec);
  return LaunchWithIProcess(spec);
}

nsresult
nsLocalHandlerApp::LaunchWithIProcess(const nsCString& aArg)
{
  nsresult rv;
  nsCOMPtr<nsIProcess> process = do_CreateInstance(NS_PROCESS_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  if (NS_FAILED(rv = process->Init(mExecutable)))
    return rv;

  const char *string = aArg.get();

  return process->Run(PR_FALSE, &string, 1);
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

