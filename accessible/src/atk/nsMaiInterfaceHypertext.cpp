






































#include "InterfaceInitFuncs.h"

#include "nsHyperTextAccessible.h"
#include "nsMai.h"
#include "nsMaiHyperlink.h"

extern "C" {

static AtkHyperlink*
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

static gint
getLinkCountCB(AtkHypertext *aText)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return -1;

    nsHyperTextAccessible* hyperText = accWrap->AsHyperText();
    NS_ENSURE_TRUE(hyperText, -1);

    return hyperText->GetLinkCount();
}

static gint
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
}

void
hypertextInterfaceInitCB(AtkHypertextIface* aIface)
{
  NS_ASSERTION(aIface, "no interface!");
  if (NS_UNLIKELY(!aIface))
    return;

  aIface->get_link = getLinkCB;
  aIface->get_n_links = getLinkCountCB;
  aIface->get_link_index = getLinkIndexCB;
}
