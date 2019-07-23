






























#include "nsPrintProgressParams.h"
#include "nsReadableUtils.h"


NS_IMPL_ISUPPORTS1(nsPrintProgressParams, nsIPrintProgressParams)

nsPrintProgressParams::nsPrintProgressParams()
{
}

nsPrintProgressParams::~nsPrintProgressParams()
{
}


NS_IMETHODIMP nsPrintProgressParams::GetDocTitle(PRUnichar * *aDocTitle)
{
  NS_ENSURE_ARG(aDocTitle);
  
  *aDocTitle = ToNewUnicode(mDocTitle);
  return NS_OK;
}

NS_IMETHODIMP nsPrintProgressParams::SetDocTitle(const PRUnichar * aDocTitle)
{
  mDocTitle = aDocTitle;
  return NS_OK;
}


NS_IMETHODIMP nsPrintProgressParams::GetDocURL(PRUnichar * *aDocURL)
{
  NS_ENSURE_ARG(aDocURL);
  
  *aDocURL = ToNewUnicode(mDocURL);
  return NS_OK;
}

NS_IMETHODIMP nsPrintProgressParams::SetDocURL(const PRUnichar * aDocURL)
{
  mDocURL = aDocURL;
  return NS_OK;
}

