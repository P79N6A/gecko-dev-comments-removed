





#include "InterfaceInitFuncs.h"

#include "nsMaiHyperlink.h"
#include "mozilla/Likely.h"

extern "C" {
static AtkHyperlink*
getHyperlinkCB(AtkHyperlinkImpl* aImpl)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aImpl));
  if (!accWrap)
    return nullptr;

  NS_ENSURE_TRUE(accWrap->IsLink(), nullptr);

  MaiHyperlink* maiHyperlink = accWrap->GetMaiHyperlink();
  NS_ENSURE_TRUE(maiHyperlink, nullptr);
  return maiHyperlink->GetAtkHyperlink();
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
