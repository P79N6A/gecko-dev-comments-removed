







































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

    nsCOMPtr<nsIAccessibleHyperLink> accHyperlink;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleHyperLink),
                            getter_AddRefs(accHyperlink));
    NS_ENSURE_TRUE(accHyperlink, nsnull);
    
    MaiHyperlink *maiHyperlink = accWrap->GetMaiHyperlink();
    NS_ENSURE_TRUE(maiHyperlink, nsnull);
    return maiHyperlink->GetAtkHyperlink();

}
