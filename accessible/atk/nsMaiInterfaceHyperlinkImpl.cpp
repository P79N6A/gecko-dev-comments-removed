





#include "InterfaceInitFuncs.h"

#include "nsMaiHyperlink.h"
#include "mozilla/Likely.h"

using namespace mozilla::a11y;

extern "C" {
static AtkHyperlink*
getHyperlinkCB(AtkHyperlinkImpl* aImpl)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aImpl));
  if (!accWrap || !GetProxy(ATK_OBJECT(aImpl)))
    return nullptr;

  if (accWrap)
    NS_ASSERTION(accWrap->IsLink(), "why isn't it a link!");

  return MAI_ATK_OBJECT(aImpl)->GetAtkHyperlink();
}
}

void
hyperlinkImplInterfaceInitCB(AtkHyperlinkImplIface *aIface)
{
  NS_ASSERTION(aIface, "no interface!");
  if (MOZ_UNLIKELY(!aIface))
    return;

  aIface->get_hyperlink = getHyperlinkCB;
}
