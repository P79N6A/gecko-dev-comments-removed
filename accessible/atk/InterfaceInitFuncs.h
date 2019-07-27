





#ifndef ATK_INTERFACE_INIT_FUNCS_H_
#define ATK_INTERFACE_INIT_FUNCS_H_

#include <atk/atk.h>

namespace mozilla {
namespace a11y {

class AccessibleWrap;

} 
} 

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




AtkObject* refAccessibleAtPointHelper(mozilla::a11y::AccessibleWrap* aAccWrap,
                                      gint aX, gint aY, AtkCoordType aCoordType);
void getExtentsHelper(mozilla::a11y::AccessibleWrap* aAccWrap,
                      gint* aX, gint* aY, gint* aWidth, gint* aHeight,
                      AtkCoordType aCoordType);

#endif 
