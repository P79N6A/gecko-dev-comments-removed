






































#ifndef __MAI_INTERFACE_HYPERLINKIMPL_H__
#define __MAI_INTERFACE_HYPERLINKIMPL_H__

#include "nsMai.h"

G_BEGIN_DECLS

void hyperlinkImplInterfaceInitCB(AtkHyperlinkImplIface *aIface);


AtkHyperlink  *getHyperlinkCB (AtkHyperlinkImpl *aImpl);

G_END_DECLS

#endif 

