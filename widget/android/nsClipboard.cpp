



#include "mozilla/dom/ContentChild.h"
#include "nsClipboard.h"
#include "nsISupportsPrimitives.h"
#include "AndroidBridge.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsXULAppAPI.h"

using namespace mozilla;
using mozilla::dom::ContentChild;

NS_IMPL_ISUPPORTS(nsClipboard, nsIClipboard)







nsClipboard::nsClipboard()
{
}

NS_IMETHODIMP
nsClipboard::SetData(nsITransferable *aTransferable,
                     nsIClipboardOwner *anOwner, int32_t aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_NOT_IMPLEMENTED;

  nsCOMPtr<nsISupports> tmp;
  uint32_t len;
  nsresult rv  = aTransferable->GetTransferData(kUnicodeMime, getter_AddRefs(tmp),
                                                &len);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISupportsString> supportsString = do_QueryInterface(tmp);
  
  NS_ENSURE_TRUE(supportsString, NS_ERROR_NOT_IMPLEMENTED);
  nsAutoString buffer;
  supportsString->GetData(buffer);

  mozilla::widget::android::Clipboard::SetClipboardText(buffer);
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::GetData(nsITransferable *aTransferable, int32_t aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_NOT_IMPLEMENTED;

  nsAutoString buffer;
  if (!AndroidBridge::Bridge())
    return NS_ERROR_NOT_IMPLEMENTED;
  if (!AndroidBridge::Bridge()->GetClipboardText(buffer))
    return NS_ERROR_UNEXPECTED;

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
nsClipboard::EmptyClipboard(int32_t aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_NOT_IMPLEMENTED;
  mozilla::widget::android::Clipboard::ClearText();
  
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(const char **aFlavorList,
                                    uint32_t aLength, int32_t aWhichClipboard,
                                    bool *aHasText)
{
  *aHasText = false;
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_NOT_IMPLEMENTED;
  *aHasText = mozilla::widget::android::Clipboard::HasText();
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsSelectionClipboard(bool *aIsSupported)
{
  *aIsSupported = false;
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsFindClipboard(bool* _retval)
{
  *_retval = false;
  return NS_OK;
}
