






































#ifndef __MAI_INTERFACE_HYPERTEXT_H__
#define __MAI_INTERFACE_HYPERTEXT_H__

#include "nsMai.h"
#include "nsMaiHyperlink.h"
#include "nsIAccessibleHyperText.h"

G_BEGIN_DECLS

void hypertextInterfaceInitCB(AtkHypertextIface *aIface);


AtkHyperlink *getLinkCB(AtkHypertext *aText, gint aLinkIndex);
gint getLinkCountCB(AtkHypertext *aText);
gint getLinkIndexCB(AtkHypertext *aText, gint aCharIndex);

G_END_DECLS

#endif 
