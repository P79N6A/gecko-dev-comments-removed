




#include "mozilla/Util.h"

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
#include "mozilla/Services.h"

#if defined(XP_WIN)
#include <ole2.h>
#endif

using namespace mozilla;

#define NS_MOZ_DATA_FROM_PRIVATEBROWSING "application/x-moz-private-browsing"

NS_IMPL_ISUPPORTS2(nsClipboardPrivacyHandler, nsIObserver, nsISupportsWeakReference)

nsresult
nsClipboardPrivacyHandler::Init()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;
  return observerService->AddObserver(this, "last-pb-context-exited", true);
}





nsresult
nsClipboardPrivacyHandler::PrepareDataForClipboard(nsITransferable * aTransferable)
{
  NS_ASSERTION(aTransferable, "clipboard given a null transferable");

  bool isPrivateData = false;
  aTransferable->GetIsPrivateData(&isPrivateData);

  nsresult rv = NS_OK;
  if (isPrivateData) {
    nsCOMPtr<nsISupportsPRBool> data = do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);
    if (data) {
      rv = data->SetData(true);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      aTransferable->AddDataFlavor(NS_MOZ_DATA_FROM_PRIVATEBROWSING);

      rv = aTransferable->SetTransferData(NS_MOZ_DATA_FROM_PRIVATEBROWSING, data, sizeof(bool));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsClipboardPrivacyHandler::Observe(nsISupports *aSubject, char const *aTopic, PRUnichar const *aData)
{
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard =
    do_GetService("@mozilla.org/widget/clipboard;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  const char * flavors[] = { NS_MOZ_DATA_FROM_PRIVATEBROWSING };
  bool haveFlavors;
  rv = clipboard->HasDataMatchingFlavors(flavors,
                                         ArrayLength(flavors),
                                         nsIClipboard::kGlobalClipboard,
                                         &haveFlavors);
  if (NS_SUCCEEDED(rv) && haveFlavors) {
#if defined(XP_WIN)
    
    
    
    
    
    NS_ENSURE_TRUE(SUCCEEDED(::OleSetClipboard(NULL)), NS_ERROR_FAILURE);
#else
    
    nsCOMPtr<nsITransferable> nullData =
      do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nullData->Init(nullptr);
    rv = clipboard->SetData(nullData, nullptr,
                            nsIClipboard::kGlobalClipboard);
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }

  return NS_OK;
}

nsresult
NS_NewClipboardPrivacyHandler(nsClipboardPrivacyHandler ** aHandler)
{
  NS_PRECONDITION(aHandler != nullptr, "null ptr");
  if (!aHandler)
    return NS_ERROR_NULL_POINTER;

  *aHandler = new nsClipboardPrivacyHandler();

  NS_ADDREF(*aHandler);
  nsresult rv = (*aHandler)->Init();
  if (NS_FAILED(rv))
    NS_RELEASE(*aHandler);

  return rv;
}
