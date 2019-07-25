






































#ifndef __MAI_INTERFACE_SELECTION_H__
#define __MAI_INTERFACE_SELECTION_H__

#include "nsMai.h"

G_BEGIN_DECLS



void selectionInterfaceInitCB(AtkSelectionIface *aIface);
gboolean addSelectionCB(AtkSelection *aSelection, gint i);
gboolean clearSelectionCB(AtkSelection *aSelection);
AtkObject *refSelectionCB(AtkSelection *aSelection, gint i);
gint getSelectionCountCB(AtkSelection *aSelection);
gboolean isChildSelectedCB(AtkSelection *aSelection, gint i);
gboolean removeSelectionCB(AtkSelection *aSelection, gint i);
gboolean selectAllSelectionCB(AtkSelection *aSelection);

G_END_DECLS

#endif 
