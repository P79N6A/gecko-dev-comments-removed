






































#ifndef __MAI_INTERFACE_COMPONENT_H__
#define __MAI_INTERFACE_COMPONENT_H__

#include "nsMai.h"

G_BEGIN_DECLS


void componentInterfaceInitCB(AtkComponentIface* aIface);
AtkObject* refAccessibleAtPointCB(AtkComponent* aComponent,
                                  gint aX, gint aY,
                                  AtkCoordType aCoordType);
void getExtentsCB(AtkComponent* aComponent,
                  gint* aX, gint* aY, gint* aWidth, gint* aHeight,
                  AtkCoordType aCoordType);



gboolean grabFocusCB(AtkComponent* aComponent);












AtkObject* refAccessibleAtPointHelper(nsAccessibleWrap* aAccWrap,
                                      gint aX, gint aY, AtkCoordType aCoordType);
void getExtentsHelper(nsAccessibleWrap* aAccWrap,
                      gint* aX, gint* aY, gint* aWidth, gint* aHeight,
                      AtkCoordType aCoordType);

G_END_DECLS

#endif 
