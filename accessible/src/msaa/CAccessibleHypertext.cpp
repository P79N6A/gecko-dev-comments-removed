







































#include "CAccessibleHypertext.h"

#include "AccessibleHypertext_i.c"

#include "nsIAccessibleHypertext.h"
#include "nsIWinAccessNode.h"

#include "nsCOMPtr.h"



STDMETHODIMP
CAccessibleHypertext::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleHypertext == iid) {
    nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryInterface(this));
    if (!hyperAcc)
      return E_NOINTERFACE;

    *ppv = NS_STATIC_CAST(IAccessibleHypertext*, this);
    (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef();
    return S_OK;
  }

  return CAccessibleText::QueryInterface(iid, ppv);
}



STDMETHODIMP
CAccessibleHypertext::get_nHyperlinks(long *aHyperlinkCount)
{
  *aHyperlinkCount = 0;

  nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryInterface(this));
  if (!hyperAcc)
    return E_FAIL;

  PRInt32 count = 0;
  nsresult rv = hyperAcc->GetLinks(&count);
  *aHyperlinkCount = count;

  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleHypertext::get_hyperlink(long aIndex,
                                    IAccessibleHyperlink **aHyperlink)
{
  *aHyperlink = NULL;

  nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryInterface(this));
  if (!hyperAcc)
    return E_FAIL;

  nsCOMPtr<nsIAccessibleHyperLink> hyperLink;
  hyperAcc->GetLink(aIndex, getter_AddRefs(hyperLink));
  if (!hyperLink)
    return E_FAIL;

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(hyperLink));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  nsresult rv =  winAccessNode->QueryNativeInterface(IID_IAccessibleHyperlink,
                                                     &instancePtr);
  *aHyperlink = NS_STATIC_CAST(IAccessibleHyperlink*, instancePtr);

  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleHypertext::get_hyperlinkIndex(long aCharIndex, long *aHyperlinkIndex)
{
  *aHyperlinkIndex = 0;

  nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryInterface(this));
  if (!hyperAcc)
    return E_FAIL;

  PRInt32 index = 0;
  nsresult rv = hyperAcc->GetLinkIndex(aCharIndex, &index);
  *aHyperlinkIndex = index;

  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

