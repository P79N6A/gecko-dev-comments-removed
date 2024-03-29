






#include "ia2AccessibleHypertext.h"

#include "AccessibleHypertext_i.c"

#include "HyperTextAccessibleWrap.h"
#include "IUnknownImpl.h"

using namespace mozilla::a11y;



STDMETHODIMP
ia2AccessibleHypertext::get_nHyperlinks(long* aHyperlinkCount)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aHyperlinkCount)
    return E_INVALIDARG;

  *aHyperlinkCount = 0;

  if (ProxyAccessible* proxy = HyperTextProxyFor(this)) {
    *aHyperlinkCount = proxy->LinkCount();
    return S_OK;
  }

  HyperTextAccessibleWrap* hyperText = static_cast<HyperTextAccessibleWrap*>(this);
  if (hyperText->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aHyperlinkCount = hyperText->LinkCount();
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleHypertext::get_hyperlink(long aLinkIndex,
                                      IAccessibleHyperlink** aHyperlink)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aHyperlink)
    return E_INVALIDARG;

  *aHyperlink = nullptr;

  AccessibleWrap* hyperLink;
  if (ProxyAccessible* proxy = HyperTextProxyFor(this)) {
    ProxyAccessible* link = proxy->LinkAt(aLinkIndex);
    if (!link)
      return E_FAIL;

    hyperLink = WrapperFor(link);
  } else {
    HyperTextAccessibleWrap* hyperText = static_cast<HyperTextAccessibleWrap*>(this);
    if (hyperText->IsDefunct())
      return CO_E_OBJNOTCONNECTED;

    hyperLink = static_cast<AccessibleWrap*>(hyperText->LinkAt(aLinkIndex));
  }

  if (!hyperLink)
    return E_FAIL;

  *aHyperlink =
    static_cast<IAccessibleHyperlink*>(hyperLink);
  (*aHyperlink)->AddRef();
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleHypertext::get_hyperlinkIndex(long aCharIndex, long* aHyperlinkIndex)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aHyperlinkIndex)
    return E_INVALIDARG;

  *aHyperlinkIndex = 0;

  if (ProxyAccessible* proxy = HyperTextProxyFor(this)) {
    *aHyperlinkIndex = proxy->LinkIndexAtOffset(aCharIndex);
    return S_OK;
  }

  HyperTextAccessibleWrap* hyperAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (hyperAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aHyperlinkIndex = hyperAcc->LinkIndexAtOffset(aCharIndex);
  return S_OK;

  A11Y_TRYBLOCK_END
}

