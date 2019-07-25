






































#include "InterfaceInitFuncs.h"

#include "nsMaiHyperlink.h"

extern "C" {
static AtkHyperlink*
getHyperlinkCB(AtkHyperlinkImpl* aImpl)
{
  nsAccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aImpl));
  if (!accWrap)
    return nsnull;

  NS_ENSURE_TRUE(accWrap->IsLink(), nsnull);

  MaiHyperlink* maiHyperlink = accWrap->GetMaiHyperlink();
  NS_ENSURE_TRUE(maiHyperlink, nsnull);
  return maiHyperlink->GetAtkHyperlink();
}
}

void
hyperlinkImplInterfaceInitCB(AtkHyperlinkImplIface *aIface)
{
  NS_ASSERTION(aIface, "no interface!");
  if (NS_UNLIKELY(!aIface))
    return;

  aIface->get_hyperlink = getHyperlinkCB;
}
