





































#include "nsClipboardHelper.h"


#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIServiceManager.h"


#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsReadableUtils.h"

NS_IMPL_ISUPPORTS1(nsClipboardHelper, nsIClipboardHelper)





nsClipboardHelper::nsClipboardHelper()
{
}

nsClipboardHelper::~nsClipboardHelper()
{
  
}





NS_IMETHODIMP
nsClipboardHelper::CopyStringToClipboard(const nsAString& aString,
                                         PRInt32 aClipboardID)
{
  nsresult rv;

  
  nsCOMPtr<nsIClipboard>
    clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(clipboard, NS_ERROR_FAILURE);

  
  
  if (nsIClipboard::kSelectionClipboard == aClipboardID) {
    PRBool clipboardSupported;
    rv = clipboard->SupportsSelectionClipboard(&clipboardSupported);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!clipboardSupported)
      return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsITransferable>
    trans(do_CreateInstance("@mozilla.org/widget/transferable;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(trans, NS_ERROR_FAILURE);

  
  rv = trans->AddDataFlavor(kUnicodeMime);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISupportsString>
    data(do_CreateInstance("@mozilla.org/supports-string;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(data, NS_ERROR_FAILURE);

  
  rv = data->SetData(aString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsISupports> genericData(do_QueryInterface(data, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(genericData, NS_ERROR_FAILURE);

  
  rv = trans->SetTransferData(kUnicodeMime, genericData,
                              aString.Length() * 2);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = clipboard->SetData(trans, nsnull, aClipboardID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsClipboardHelper::CopyString(const nsAString& aString)
{
  nsresult rv;

  
  rv = CopyStringToClipboard(aString, nsIClipboard::kGlobalClipboard);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  CopyStringToClipboard(aString, nsIClipboard::kSelectionClipboard);

  return NS_OK;
}
