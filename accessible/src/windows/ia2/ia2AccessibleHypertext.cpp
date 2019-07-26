






#include "ia2AccessibleHypertext.h"

#include "AccessibleHypertext_i.c"

#include "HyperTextAccessibleWrap.h"

using namespace mozilla::a11y;



STDMETHODIMP
ia2AccessibleHypertext::get_nHyperlinks(long* aHyperlinkCount)
{
  A11Y_TRYBLOCK_BEGIN

  *aHyperlinkCount = 0;

  HyperTextAccessibleWrap* hyperText = static_cast<HyperTextAccessibleWrap*>(this);
  if (hyperText->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aHyperlinkCount = hyperText->GetLinkCount();
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleHypertext::get_hyperlink(long aLinkIndex,
                                      IAccessibleHyperlink** aHyperlink)
{
  A11Y_TRYBLOCK_BEGIN

  *aHyperlink = NULL;

  HyperTextAccessibleWrap* hyperText = static_cast<HyperTextAccessibleWrap*>(this);
  if (hyperText->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* hyperLink = hyperText->GetLinkAt(aLinkIndex);
  if (!hyperText)
    return E_FAIL;

  *aHyperlink =
    static_cast<IAccessibleHyperlink*>(static_cast<AccessibleWrap*>(hyperLink));
  (*aHyperlink)->AddRef();
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleHypertext::get_hyperlinkIndex(long aCharIndex, long* aHyperlinkIndex)
{
  A11Y_TRYBLOCK_BEGIN

  *aHyperlinkIndex = 0;

  HyperTextAccessibleWrap* hyperAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (hyperAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aHyperlinkIndex = hyperAcc->GetLinkIndexAtOffset(aCharIndex);
  return S_OK;

  A11Y_TRYBLOCK_END
}

