





































#include "nsNativeAppSupportBase.h"

nsNativeAppSupportBase::nsNativeAppSupportBase()
{
}

nsNativeAppSupportBase::~nsNativeAppSupportBase()
{
}

NS_IMPL_ISUPPORTS1(nsNativeAppSupportBase, nsINativeAppSupport)


NS_IMETHODIMP
nsNativeAppSupportBase::Start( PRBool *result )
{
  *result = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::Enable()
{
  return NS_OK;
}


NS_IMETHODIMP
nsNativeAppSupportBase::Stop( PRBool *result )
{
  *result = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::Quit()
{
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::ReOpen()
{
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBase::OnLastWindowClosing() {
  return NS_OK;
}
