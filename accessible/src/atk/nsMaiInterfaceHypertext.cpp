





#include "InterfaceInitFuncs.h"

#include "HyperTextAccessible.h"
#include "nsMai.h"
#include "nsMaiHyperlink.h"

extern "C" {

static AtkHyperlink*
getLinkCB(AtkHypertext *aText, gint aLinkIndex)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
  if (!accWrap)
    return nullptr;

  HyperTextAccessible* hyperText = accWrap->AsHyperText();
  NS_ENSURE_TRUE(hyperText, nullptr);

  Accessible* hyperLink = hyperText->GetLinkAt(aLinkIndex);
  if (!hyperLink)
    return nullptr;

  AtkObject* hyperLinkAtkObj = AccessibleWrap::GetAtkObject(hyperLink);
  AccessibleWrap* accChild = GetAccessibleWrap(hyperLinkAtkObj);
  NS_ENSURE_TRUE(accChild, nullptr);

  MaiHyperlink *maiHyperlink = accChild->GetMaiHyperlink();
  NS_ENSURE_TRUE(maiHyperlink, nullptr);
  return maiHyperlink->GetAtkHyperlink();
}

static gint
getLinkCountCB(AtkHypertext *aText)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
  if (!accWrap)
    return -1;

  HyperTextAccessible* hyperText = accWrap->AsHyperText();
  NS_ENSURE_TRUE(hyperText, -1);

  return hyperText->GetLinkCount();
}

static gint
getLinkIndexCB(AtkHypertext *aText, gint aCharIndex)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
  if (!accWrap)
    return -1;

  HyperTextAccessible* hyperText = accWrap->AsHyperText();
  NS_ENSURE_TRUE(hyperText, -1);

  int32_t index = -1;
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
