





































#include "nsClipboardCE.h"

#include <winuserm.h>

nsClipboard::nsClipboard()
{
}

nsClipboard::~nsClipboard()
{
}

NS_IMETHODIMP
nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable *aTransferable,
				    PRInt32 aWhichClipboard)
{
  return NS_ERROR_FAILURE;
}
