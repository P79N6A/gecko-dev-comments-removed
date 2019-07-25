






































#include "nsMaiInterfaceHypertext.h"
#include "nsIAccessibleDocument.h"
#include "nsHyperTextAccessible.h"

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
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return nsnull;

    nsHyperTextAccessible* hyperText = accWrap->AsHyperText();
    NS_ENSURE_TRUE(hyperText, nsnull);

    nsAccessible* hyperLink = hyperText->GetLinkAt(aLinkIndex);
    if (!hyperLink)
        return nsnull;

    AtkObject* hyperLinkAtkObj = nsAccessibleWrap::GetAtkObject(hyperLink);
    nsAccessibleWrap *accChild = GetAccessibleWrap(hyperLinkAtkObj);
    NS_ENSURE_TRUE(accChild, nsnull);

    MaiHyperlink *maiHyperlink = accChild->GetMaiHyperlink();
    NS_ENSURE_TRUE(maiHyperlink, nsnull);
    return maiHyperlink->GetAtkHyperlink();
}

gint
getLinkCountCB(AtkHypertext *aText)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return -1;

    nsHyperTextAccessible* hyperText = accWrap->AsHyperText();
    NS_ENSURE_TRUE(hyperText, -1);

    return hyperText->GetLinkCount();
}

gint
getLinkIndexCB(AtkHypertext *aText, gint aCharIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return -1;

    nsHyperTextAccessible* hyperText = accWrap->AsHyperText();
    NS_ENSURE_TRUE(hyperText, -1);

    PRInt32 index = -1;
    nsresult rv = hyperText->GetLinkIndexAtOffset(aCharIndex, &index);
    NS_ENSURE_SUCCESS(rv, -1);

    return index;
}
