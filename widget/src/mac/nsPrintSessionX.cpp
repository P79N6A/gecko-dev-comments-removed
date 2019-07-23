





































#include "nsPrintSessionX.h"





NS_IMPL_ISUPPORTS_INHERITED1(nsPrintSessionX, 
                             nsPrintSession, 
                             nsIPrintSessionX)
                             

nsPrintSessionX::nsPrintSessionX()
{
}


nsPrintSessionX::~nsPrintSessionX()
{
  if (mSession) {
    ::PMRelease(mSession);
    mSession = nsnull;
  }
}


nsresult nsPrintSessionX::Init()
{
  nsresult rv = nsPrintSession::Init();
  if (NS_FAILED(rv))
    return rv;
  
  OSStatus status = ::PMCreateSession(&mSession);
  if (status != noErr)
    return NS_ERROR_FAILURE;
    
  return NS_OK;
}



NS_IMETHODIMP nsPrintSessionX::GetNativeSession(PMPrintSession *aNativeSession)
{
  NS_ENSURE_ARG_POINTER(aNativeSession);
  *aNativeSession = nsnull;
  
  if (!mSession)
    return NS_ERROR_NOT_INITIALIZED;
    
  *aNativeSession = mSession;
  return NS_OK;
}
