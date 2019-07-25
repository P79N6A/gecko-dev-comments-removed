






































#ifndef __MAI_INTERFACE_ACTION_H__
#define __MAI_INTERFACE_ACTION_H__

#include "nsMai.h"

G_BEGIN_DECLS


void actionInterfaceInitCB(AtkActionIface *aIface);
gboolean doActionCB(AtkAction *aAction, gint aActionIndex);
gint getActionCountCB(AtkAction *aAction);
const gchar *getActionDescriptionCB(AtkAction *aAction, gint aActionIndex);
const gchar *getActionNameCB(AtkAction *aAction, gint aActionIndex);
const gchar *getKeyBindingCB(AtkAction *aAction, gint aActionIndex);

G_END_DECLS

#endif 
