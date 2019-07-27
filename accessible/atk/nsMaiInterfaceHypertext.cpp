





#include "InterfaceInitFuncs.h"

#include "Accessible-inl.h"
#include "HyperTextAccessible.h"
#include "nsMai.h"
#include "nsMaiHyperlink.h"
#include "ProxyAccessible.h"
#include "mozilla/Likely.h"


using namespace mozilla::a11y;

extern "C" {

static AtkHyperlink*
getLinkCB(AtkHypertext *aText, gint aLinkIndex)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
  if (accWrap) {
    HyperTextAccessible* hyperText = accWrap->AsHyperText();
    NS_ENSURE_TRUE(hyperText, nullptr);

    Accessible* hyperLink = hyperText->LinkAt(aLinkIndex);
    if (!hyperLink) {
      return nullptr;
    }

    AtkObject* hyperLinkAtkObj = AccessibleWrap::GetAtkObject(hyperLink);
    NS_ENSURE_TRUE(IS_MAI_OBJECT(hyperLinkAtkObj), nullptr);

    return MAI_ATK_OBJECT(hyperLinkAtkObj)->GetAtkHyperlink();
  }

  if (ProxyAccessible* proxy = GetProxy(ATK_OBJECT(aText))) {
    ProxyAccessible* proxyLink = proxy->LinkAt(aLinkIndex);
    if (proxyLink) {
      NS_WARNING("IMPLEMENT ME! See bug 1146518.");
      
    }
    return nullptr;
  }

  return nullptr;
}

static gint
getLinkCountCB(AtkHypertext *aText)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
  if (accWrap) {
    HyperTextAccessible* hyperText = accWrap->AsHyperText();
    NS_ENSURE_TRUE(hyperText, -1);
    return hyperText->LinkCount();
  }

  if (ProxyAccessible* proxy = GetProxy(ATK_OBJECT(aText))) {
    return proxy->LinkCount();
  }

  return -1;
}

static gint
getLinkIndexCB(AtkHypertext *aText, gint aCharIndex)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
  if (accWrap) {
    HyperTextAccessible* hyperText = accWrap->AsHyperText();
    NS_ENSURE_TRUE(hyperText, -1);

    return hyperText->LinkIndexAtOffset(aCharIndex);
  }

  if (ProxyAccessible* proxy = GetProxy(ATK_OBJECT(aText))) {
    return proxy->LinkIndexAtOffset(aCharIndex);
  }

  return -1;
}
}

void
hypertextInterfaceInitCB(AtkHypertextIface* aIface)
{
  NS_ASSERTION(aIface, "no interface!");
  if (MOZ_UNLIKELY(!aIface))
    return;

  aIface->get_link = getLinkCB;
  aIface->get_n_links = getLinkCountCB;
  aIface->get_link_index = getLinkIndexCB;
}
