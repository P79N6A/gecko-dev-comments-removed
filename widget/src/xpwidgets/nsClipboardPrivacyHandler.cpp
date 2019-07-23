





































#include "nsClipboardPrivacyHandler.h"
#include "nsITransferable.h"
#include "nsISupportsPrimitives.h"
#include "nsIObserverService.h"
#include "nsIClipboard.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsLiteralString.h"
#include "nsNetCID.h"
#include "nsXPCOM.h"

#define NS_MOZ_DATA_FROM_PRIVATEBROWSING "application/x-moz-private-browsing"

NS_IMPL_ISUPPORTS2(nsClipboardPrivacyHandler, nsIObserver, nsISupportsWeakReference)

nsresult
nsClipboardPrivacyHandler::Init()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = observerService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_TRUE);
  return rv;
}





nsresult
nsClipboardPrivacyHandler::PrepareDataForClipboard(nsITransferable * aTransferable)
{
  NS_ASSERTION(aTransferable, "clipboard given a null transferable");

  nsresult rv = NS_OK;
  if (InPrivateBrowsing()) {
    nsCOMPtr<nsISupportsPRBool> data = do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);
    if (data) {
      rv = data->SetData(PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aTransferable->AddDataFlavor(NS_MOZ_DATA_FROM_PRIVATEBROWSING);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aTransferable->SetTransferData(NS_MOZ_DATA_FROM_PRIVATEBROWSING, data, sizeof(PRBool));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsClipboardPrivacyHandler::Observe(nsISupports *aSubject, char const *aTopic, PRUnichar const *aData)
{
  if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData)) {
    nsresult rv;
    nsCOMPtr<nsIClipboard> clipboard =
      do_GetService("@mozilla.org/widget/clipboard;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    const char * flavors[] = { NS_MOZ_DATA_FROM_PRIVATEBROWSING };
    PRBool haveFlavors;
    rv = clipboard->HasDataMatchingFlavors(flavors,
                                           NS_ARRAY_LENGTH(flavors),
                                           nsIClipboard::kGlobalClipboard,
                                           &haveFlavors);
    if (NS_SUCCEEDED(rv) && haveFlavors) {
      
      nsCOMPtr<nsITransferable> nullData =
        do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = clipboard->SetData(nullData, nsnull,
                              nsIClipboard::kGlobalClipboard);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

PRBool
nsClipboardPrivacyHandler::InPrivateBrowsing()
{
  PRBool inPrivateBrowsingMode = PR_FALSE;
  if (!mPBService)
    mPBService = do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (mPBService)
    mPBService->GetPrivateBrowsingEnabled(&inPrivateBrowsingMode);
  return inPrivateBrowsingMode;
}

nsresult
NS_NewClipboardPrivacyHandler(nsClipboardPrivacyHandler ** aHandler)
{
  NS_PRECONDITION(aHandler != nsnull, "null ptr");
  if (!aHandler)
    return NS_ERROR_NULL_POINTER;

  *aHandler = new nsClipboardPrivacyHandler();
  if (!*aHandler)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aHandler);
  nsresult rv = (*aHandler)->Init();
  if (NS_FAILED(rv))
    NS_RELEASE(*aHandler);

  return rv;
}
