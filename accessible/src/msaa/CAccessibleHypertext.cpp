







































#include "CAccessibleHypertext.h"

#include "AccessibleHypertext_i.c"

#include "nsIAccessibleHypertext.h"
#include "nsIWinAccessNode.h"
#include "nsAccessNodeWrap.h"

#include "nsCOMPtr.h"



STDMETHODIMP
CAccessibleHypertext::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;
  if (IID_IAccessibleHypertext == iid) {
    nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryInterface(this));
    if (!hyperAcc)
      return E_NOINTERFACE;

    *ppv = static_cast<IAccessibleHypertext*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return CAccessibleText::QueryInterface(iid, ppv);
}



STDMETHODIMP
CAccessibleHypertext::get_nHyperlinks(long *aHyperlinkCount)
{
__try {
  *aHyperlinkCount = 0;

  nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryInterface(this));
  if (!hyperAcc)
    return E_FAIL;

  PRInt32 count = 0;
  nsresult rv = hyperAcc->GetLinks(&count);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  *aHyperlinkCount = count;
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleHypertext::get_hyperlink(long aIndex,
                                    IAccessibleHyperlink **aHyperlink)
{
__try {
  *aHyperlink = NULL;

  nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryInterface(this));
  if (!hyperAcc)
    return E_FAIL;

  nsCOMPtr<nsIAccessibleHyperLink> hyperLink;
  nsresult rv = hyperAcc->GetLink(aIndex, getter_AddRefs(hyperLink));
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(hyperLink));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  rv =  winAccessNode->QueryNativeInterface(IID_IAccessibleHyperlink,
                                            &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aHyperlink = static_cast<IAccessibleHyperlink*>(instancePtr);
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleHypertext::get_hyperlinkIndex(long aCharIndex, long *aHyperlinkIndex)
{
__try {
  *aHyperlinkIndex = 0;

  nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryInterface(this));
  if (!hyperAcc)
    return E_FAIL;

  PRInt32 index = 0;
  nsresult rv = hyperAcc->GetLinkIndex(aCharIndex, &index);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  *aHyperlinkIndex = index;
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

