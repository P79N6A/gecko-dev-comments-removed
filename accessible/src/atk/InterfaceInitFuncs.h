





#ifndef ATK_INTERFACE_INIT_FUNCS_H_
#define ATK_INTERFACE_INIT_FUNCS_H_

#include <atk/atk.h>

class nsAccessibleWrap;

extern "C" {
void actionInterfaceInitCB(AtkActionIface* aIface);
void componentInterfaceInitCB(AtkComponentIface* aIface);
void documentInterfaceInitCB(AtkDocumentIface *aIface);
void editableTextInterfaceInitCB(AtkEditableTextIface* aIface);
void hyperlinkImplInterfaceInitCB(AtkHyperlinkImplIface *aIface);
void hypertextInterfaceInitCB(AtkHypertextIface* aIface);
void imageInterfaceInitCB(AtkImageIface* aIface);
void selectionInterfaceInitCB(AtkSelectionIface* aIface);
void tableInterfaceInitCB(AtkTableIface *aIface);
void textInterfaceInitCB(AtkTextIface* aIface);
void valueInterfaceInitCB(AtkValueIface *aIface);
}




AtkObject* refAccessibleAtPointHelper(nsAccessibleWrap* aAccWrap,
                                      gint aX, gint aY, AtkCoordType aCoordType);
void getExtentsHelper(nsAccessibleWrap* aAccWrap,
                      gint* aX, gint* aY, gint* aWidth, gint* aHeight,
                      AtkCoordType aCoordType);

#endif 
