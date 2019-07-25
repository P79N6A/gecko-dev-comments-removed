







































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
nsLocalHandlerApp::SetDetailedDescription(const nsAString & aDescription)
{
  mDetailedDescription.Assign(aDescription);

  return NS_OK;
}

NS_IMETHODIMP
nsLocalHandlerApp::GetDetailedDescription(nsAString& aDescription)
{
  aDescription.Assign(mDetailedDescription);
  
  return NS_OK;
}

NS_IMETHODIMP
nsLocalHandlerApp::Equals(nsIHandlerApp *aHandlerApp, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aHandlerApp);

  *_retval = PR_FALSE;

  
  nsCOMPtr <nsILocalHandlerApp> localHandlerApp = do_QueryInterface(aHandlerApp);
  if (!localHandlerApp)
    return NS_OK;

  
  
  nsCOMPtr<nsIFile> executable;
  nsresult rv = localHandlerApp->GetExecutable(getter_AddRefs(executable));
  if (NS_FAILED(rv))
    return rv;

  
  if (!executable && !mExecutable) {
    *_retval = PR_TRUE;
    return NS_OK;
  }

  
  if (!mExecutable || !executable)
    return NS_OK;

  
  PRUint32 len;
  localHandlerApp->GetParameterCount(&len);
  if (mParameters.Length() != len)
    return NS_OK;

  
  for (PRUint32 idx = 0; idx < mParameters.Length(); idx++) {
    nsAutoString param;
    if (NS_FAILED(localHandlerApp->GetParameter(idx, param)) ||
        !param.Equals(mParameters[idx]))
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





NS_IMETHODIMP
nsLocalHandlerApp::GetExecutable(nsIFile **aExecutable)
{
  NS_IF_ADDREF(*aExecutable = mExecutable);
  return NS_OK;
}

NS_IMETHODIMP
nsLocalHandlerApp::SetExecutable(nsIFile *aExecutable)
{
  mExecutable = aExecutable;
  return NS_OK;
}


NS_IMETHODIMP
nsLocalHandlerApp::GetParameterCount(PRUint32 *aParameterCount)
{
  *aParameterCount = mParameters.Length();
  return NS_OK;
}


NS_IMETHODIMP
nsLocalHandlerApp::ClearParameters()
{
  mParameters.Clear();
  return NS_OK;
}


NS_IMETHODIMP
nsLocalHandlerApp::AppendParameter(const nsAString & aParam)
{
  mParameters.AppendElement(aParam);
  return NS_OK;
}


NS_IMETHODIMP
nsLocalHandlerApp::GetParameter(PRUint32 parameterIndex, nsAString & _retval)
{
  if (mParameters.Length() <= parameterIndex)
    return NS_ERROR_INVALID_ARG;

  _retval.Assign(mParameters[parameterIndex]);
  return NS_OK;
}


NS_IMETHODIMP
nsLocalHandlerApp::ParameterExists(const nsAString & aParam, PRBool *_retval)
{
  *_retval = mParameters.Contains(aParam);
  return NS_OK;
}
