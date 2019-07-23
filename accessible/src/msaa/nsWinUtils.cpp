







































#include "nsWinUtils.h"

#include "nsAccessibleWrap.h"
#include "nsIWinAccessNode.h"
#include "nsArrayUtils.h"

HRESULT
nsWinUtils::ConvertToIA2Array(nsIArray *aGeckoArray, IUnknown ***aIA2Array,
                              long *aIA2ArrayLen)
{
  *aIA2Array = NULL;
  *aIA2ArrayLen = 0;

  if (!aGeckoArray)
    return S_FALSE;

  PRUint32 length = 0;
  nsresult rv = aGeckoArray->GetLength(&length);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  if (length == 0)
    return S_FALSE;

  *aIA2Array =
    static_cast<IUnknown**>(nsMemory::Alloc((length) * sizeof(IUnknown*)));
  if (!*aIA2Array)
    return E_OUTOFMEMORY;

  PRUint32 idx = 0;
  for (; idx < length; ++idx) {
    nsCOMPtr<nsIWinAccessNode> winAccessNode =
      do_QueryElementAt(aGeckoArray, idx, &rv);
    if (NS_FAILED(rv))
      break;

    void *instancePtr = NULL;
    nsresult rv = winAccessNode->QueryNativeInterface(IID_IUnknown,
                                                      &instancePtr);
    if (NS_FAILED(rv))
      break;

    (*aIA2Array)[idx] = static_cast<IUnknown*>(instancePtr);
  }

  if (NS_FAILED(rv)) {
    for (PRUint32 idx2 = 0; idx2 < idx; idx2++) {
      (*aIA2Array)[idx2]->Release();
      (*aIA2Array)[idx2] = NULL;
    }

    nsMemory::Free(*aIA2Array);
    return GetHRESULT(rv);
  }

  *aIA2ArrayLen = length;
  return S_OK;
}
