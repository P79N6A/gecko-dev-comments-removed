







































#include "CAccessibleHypertext.h"

#include "AccessibleHypertext_i.c"

#include "nsHyperTextAccessible.h"



STDMETHODIMP
CAccessibleHypertext::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;
  if (IID_IAccessibleHypertext == iid) {
    nsCOMPtr<nsIAccessibleHyperText> hyperAcc(do_QueryObject(this));
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

  nsRefPtr<nsHyperTextAccessible> hyperText = do_QueryObject(this);
  if (hyperText->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aHyperlinkCount = hyperText->GetLinkCount();
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleHypertext::get_hyperlink(long aLinkIndex,
                                    IAccessibleHyperlink **aHyperlink)
{
__try {
  *aHyperlink = NULL;

  nsRefPtr<nsHyperTextAccessible> hyperText = do_QueryObject(this);
  if (hyperText->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAccessible* hyperLink = hyperText->GetLinkAt(aLinkIndex);
  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryObject(hyperLink));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  nsresult rv =  winAccessNode->QueryNativeInterface(IID_IAccessibleHyperlink,
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

  nsRefPtr<nsHyperTextAccessible> hyperAcc(do_QueryObject(this));
  if (hyperAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aHyperlinkIndex = hyperAcc->GetLinkIndexAtOffset(aCharIndex);
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

