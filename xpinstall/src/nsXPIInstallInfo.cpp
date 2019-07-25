





































#include "nsXPIInstallInfo.h"

NS_IMPL_ISUPPORTS1(nsXPIInstallInfo, nsIXPIInstallInfo)

nsXPIInstallInfo::nsXPIInstallInfo(nsIDOMWindow *aOriginatingWindow,
                                   nsIURI *aOriginatingURI,
                                   nsXPITriggerInfo *aTriggerInfo,
                                   PRUint32 aChromeType)
  : mOriginatingWindow(aOriginatingWindow), mOriginatingURI(aOriginatingURI),
    mTriggerInfo(aTriggerInfo), mChromeType(aChromeType)
{
}

nsXPIInstallInfo::~nsXPIInstallInfo()
{
    delete mTriggerInfo;
}


NS_IMETHODIMP
nsXPIInstallInfo::GetTriggerInfo(nsXPITriggerInfo * *aTriggerInfo)
{
    *aTriggerInfo = mTriggerInfo;
    return NS_OK;
}

NS_IMETHODIMP
nsXPIInstallInfo::SetTriggerInfo(nsXPITriggerInfo * aTriggerInfo)
{
    mTriggerInfo = aTriggerInfo;
    return NS_OK;
}


NS_IMETHODIMP
nsXPIInstallInfo::GetOriginatingWindow(nsIDOMWindow * *aOriginatingWindow)
{
    NS_IF_ADDREF(*aOriginatingWindow = mOriginatingWindow);
    return NS_OK;
}


NS_IMETHODIMP
nsXPIInstallInfo::GetOriginatingURI(nsIURI * *aOriginatingURI)
{
    NS_IF_ADDREF(*aOriginatingURI = mOriginatingURI);
    return NS_OK;
}


NS_IMETHODIMP
nsXPIInstallInfo::GetChromeType(PRUint32 *aChromeType)
{
    *aChromeType = mChromeType;
    return NS_OK;
}
