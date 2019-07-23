







































#include "nsMaiInterfaceHypertext.h"
#include "nsIAccessibleDocument.h"
#include "nsPIAccessNode.h"

void
hypertextInterfaceInitCB(AtkHypertextIface *aIface)
{
    g_return_if_fail(aIface != NULL);

    aIface->get_link = getLinkCB;
    aIface->get_n_links = getLinkCountCB;
    aIface->get_link_index = getLinkIndexCB;
}

AtkHyperlink *
getLinkCB(AtkHypertext *aText, gint aLinkIndex)
{
    nsresult rv;
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    NS_ENSURE_TRUE(accWrap, nsnull);

    nsCOMPtr<nsIAccessibleHyperText> hyperText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleHyperText),
                            getter_AddRefs(hyperText));
    NS_ENSURE_TRUE(hyperText, nsnull);

    nsCOMPtr<nsIAccessibleHyperLink> hyperLink;
    rv = hyperText->GetLink(aLinkIndex, getter_AddRefs(hyperLink));
    if (NS_FAILED(rv) || !hyperLink)
        return nsnull;

    nsCOMPtr<nsIAccessible> hyperLinkAcc(do_QueryInterface(hyperLink));
    void *hyperLinkAtkObj = nsnull;
    hyperLinkAcc->GetNativeInterface(&hyperLinkAtkObj);
    nsAccessibleWrap *accChild = GetAccessibleWrap(ATK_OBJECT(hyperLinkAtkObj));
    NS_ENSURE_TRUE(accChild, nsnull);

    MaiHyperlink *maiHyperlink = accChild->GetMaiHyperlink();
    NS_ENSURE_TRUE(maiHyperlink, nsnull);
    return maiHyperlink->GetAtkHyperlink();
}

gint
getLinkCountCB(AtkHypertext *aText)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    NS_ENSURE_TRUE(accWrap, -1);

    nsCOMPtr<nsIAccessibleHyperText> accHyperlink;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleHyperText),
                            getter_AddRefs(accHyperlink));
    NS_ENSURE_TRUE(accHyperlink, -1);

    PRInt32 count = -1;
    nsresult rv = accHyperlink->GetLinks(&count);
    NS_ENSURE_SUCCESS(rv, -1);

    return count;
}

gint
getLinkIndexCB(AtkHypertext *aText, gint aCharIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    NS_ENSURE_TRUE(accWrap, -1);

    nsCOMPtr<nsIAccessibleHyperText> accHyperlink;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleHyperText),
                            getter_AddRefs(accHyperlink));
    NS_ENSURE_TRUE(accHyperlink, -1);

    PRInt32 index = -1;
    nsresult rv = accHyperlink->GetLinkIndex(aCharIndex, &index);
    NS_ENSURE_SUCCESS(rv, -1);

    return index;
}
