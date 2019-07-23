







































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
    NS_ENSURE_TRUE(accWrap, nsnull);

    nsCOMPtr<nsIAccessibleHyperLink> accHyperlink;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleHyperLink),
                            getter_AddRefs(accHyperlink));
    NS_ENSURE_TRUE(accHyperlink, nsnull);
    
    MaiHyperlink *maiHyperlink = new MaiHyperlink(accHyperlink);
    return maiHyperlink->GetAtkHyperlink();

}
