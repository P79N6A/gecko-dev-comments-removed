







































#ifndef __MAI_INTERFACE_VALUE_H__
#define __MAI_INTERFACE_VALUE_H__

#include "nsMai.h"
#include "nsIAccessibleValue.h"

G_BEGIN_DECLS


void valueInterfaceInitCB(AtkValueIface *aIface);
void getCurrentValueCB(AtkValue *obj, GValue *value);
void getMaximumValueCB(AtkValue *obj, GValue *value);
void getMinimumValueCB(AtkValue *obj, GValue *value);
void getMinimumIncrementCB(AtkValue *obj, GValue *minIncrement);
gboolean setCurrentValueCB(AtkValue *obj, const GValue *value);

G_END_DECLS

#endif 
