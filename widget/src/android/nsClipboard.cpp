



































#include "nsClipboard.h"
#include "nsISupportsPrimitives.h"
#include "AndroidBridge.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard)







nsClipboard::nsClipboard()
{
}

NS_IMETHODIMP
nsClipboard::SetData(nsITransferable *aTransferable,
                     nsIClipboardOwner *anOwner, PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_NOT_IMPLEMENTED;
  if (!AndroidBridge::Bridge())
    return NS_ERROR_NOT_IMPLEMENTED;

  nsCOMPtr<nsISupports> tmp;
  PRUint32 len;
  nsresult rv  = aTransferable->GetTransferData(kUnicodeMime, getter_AddRefs(tmp),
                                                &len);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISupportsString> supportsString = do_QueryInterface(tmp);
  
  NS_ENSURE_TRUE(supportsString, NS_ERROR_NOT_IMPLEMENTED);
  nsAutoString buffer;
  supportsString->GetData(buffer);
  AndroidBridge::Bridge()->SetClipboardText(buffer);
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::GetData(nsITransferable *aTransferable, PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_NOT_IMPLEMENTED;
  if (!AndroidBridge::Bridge())
    return NS_ERROR_NOT_IMPLEMENTED;

  nsAutoString buffer;
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
                                      buffer.Length() * sizeof(PRUnichar));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_NOT_IMPLEMENTED;
  if (AndroidBridge::Bridge())
    AndroidBridge::Bridge()->EmptyClipboard();
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(const char **aFlavorList,
                                    PRUint32 aLength, PRInt32 aWhichClipboard,
                                    PRBool *aHasText NS_OUTPARAM)
{
  *aHasText = PR_FALSE;
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_NOT_IMPLEMENTED;
  if (AndroidBridge::Bridge())
    *aHasText = AndroidBridge::Bridge()->ClipboardHasText();
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsSelectionClipboard(PRBool *aIsSupported NS_OUTPARAM)
{
  *aIsSupported = PR_FALSE;
  return NS_OK;
}

