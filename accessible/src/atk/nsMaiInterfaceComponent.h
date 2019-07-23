







































#ifndef __MAI_INTERFACE_COMPONENT_H__
#define __MAI_INTERFACE_COMPONENT_H__

#include "nsMai.h"

G_BEGIN_DECLS


void componentInterfaceInitCB(AtkComponentIface *aIface);
AtkObject *refAccessibleAtPointCB(AtkComponent *aComponent,
                                  gint aAccX, gint aAccY,
                                  AtkCoordType aCoordType);
void getExtentsCB(AtkComponent *aComponent,
                  gint *aAccX, gint *aAccY,
                  gint *aAccWidth, gint *aAccHeight,
                  AtkCoordType aCoordType);



gboolean grabFocusCB(AtkComponent *aComponent);













G_END_DECLS

#endif 
