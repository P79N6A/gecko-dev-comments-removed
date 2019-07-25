






































#include "nsMaiInterfaceHyperlinkImpl.h"
#include "nsMaiHyperlink.h"

void
hyperlinkImplInterfaceInitCB(AtkHyperlinkImplIface *aIface)
{
    g_return_if_fail(aIface != NULL);

    aIface->get_hyperlink = getHyperlinkCB;
}

AtkHyperlink*
getHyperlinkCB(AtkHyperlinkImpl *aImpl)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aImpl));
    if (!accWrap)
        return nsnull;

    NS_ENSURE_TRUE(accWrap->IsHyperLink(), nsnull);

    MaiHyperlink *maiHyperlink = accWrap->GetMaiHyperlink();
    NS_ENSURE_TRUE(maiHyperlink, nsnull);
    return maiHyperlink->GetAtkHyperlink();

}
