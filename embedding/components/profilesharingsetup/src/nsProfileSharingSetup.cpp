





































#include "nsProfileSharingSetup.h"





nsProfileSharingSetup::nsProfileSharingSetup() :
  mSharingGloballyEnabled(PR_FALSE)
{
}

nsProfileSharingSetup::~nsProfileSharingSetup()
{
}





NS_IMPL_ISUPPORTS1(nsProfileSharingSetup,
                   nsIProfileSharingSetup)






NS_IMETHODIMP nsProfileSharingSetup::EnableSharing(const nsAString& aClientName)
{
  mSharingGloballyEnabled = PR_TRUE;
  mClientName = aClientName;
  return NS_OK;
}


NS_IMETHODIMP nsProfileSharingSetup::GetIsSharingEnabled(PRBool *aSharingEnabled)
{
  NS_ENSURE_ARG_POINTER(aSharingEnabled);
  *aSharingEnabled = mSharingGloballyEnabled;
  return NS_OK;
}


NS_IMETHODIMP nsProfileSharingSetup::GetClientName(nsAString& aClientName)
{
  aClientName = mClientName;
  return NS_OK;
}
