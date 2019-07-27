



#include "mozilla/dom/ContentChild.h"
#include "nsClipboardProxy.h"
#include "nsISupportsPrimitives.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsXULAppAPI.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(nsClipboardProxy, nsIClipboard, nsIClipboardProxy)

nsClipboardProxy::nsClipboardProxy()
  : mClipboardCaps(false, false)
{
}

NS_IMETHODIMP
nsClipboardProxy::SetData(nsITransferable *aTransferable,
                          nsIClipboardOwner *anOwner, int32_t aWhichClipboard)
{
  nsCOMPtr<nsISupports> tmp;
  uint32_t len;
  nsresult rv  = aTransferable->GetTransferData(kUnicodeMime, getter_AddRefs(tmp),
                                                &len);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISupportsString> supportsString = do_QueryInterface(tmp);
  
  NS_ENSURE_TRUE(supportsString, NS_ERROR_NOT_IMPLEMENTED);
  nsAutoString buffer;
  supportsString->GetData(buffer);

  bool isPrivateData = false;
  aTransferable->GetIsPrivateData(&isPrivateData);
  ContentChild::GetSingleton()->SendSetClipboardText(buffer, isPrivateData,
                                                     aWhichClipboard);

  return NS_OK;
}

NS_IMETHODIMP
nsClipboardProxy::GetData(nsITransferable *aTransferable, int32_t aWhichClipboard)
{
  nsAutoString buffer;
  ContentChild::GetSingleton()->SendGetClipboardText(aWhichClipboard, &buffer);

  nsresult rv;
  nsCOMPtr<nsISupportsString> dataWrapper =
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dataWrapper->SetData(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  aTransferable->AddDataFlavor(kUnicodeMime);

  nsCOMPtr<nsISupports> nsisupportsDataWrapper =
    do_QueryInterface(dataWrapper);
  rv = aTransferable->SetTransferData(kUnicodeMime, nsisupportsDataWrapper,
                                      buffer.Length() * sizeof(char16_t));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsClipboardProxy::EmptyClipboard(int32_t aWhichClipboard)
{
  ContentChild::GetSingleton()->SendEmptyClipboard(aWhichClipboard);
  return NS_OK;
}

NS_IMETHODIMP
nsClipboardProxy::HasDataMatchingFlavors(const char **aFlavorList,
                                    uint32_t aLength, int32_t aWhichClipboard,
                                    bool *aHasText)
{
  *aHasText = false;
  ContentChild::GetSingleton()->SendClipboardHasText(aWhichClipboard, aHasText);
  return NS_OK;
}

NS_IMETHODIMP
nsClipboardProxy::SupportsSelectionClipboard(bool *aIsSupported)
{
  *aIsSupported = mClipboardCaps.supportsSelectionClipboard();
  return NS_OK;
}


NS_IMETHODIMP
nsClipboardProxy::SupportsFindClipboard(bool *aIsSupported)
{
  *aIsSupported = mClipboardCaps.supportsFindClipboard();
  return NS_OK;
}

void
nsClipboardProxy::SetCapabilities(const ClipboardCapabilities& aClipboardCaps)
{
  mClipboardCaps = aClipboardCaps;
}
