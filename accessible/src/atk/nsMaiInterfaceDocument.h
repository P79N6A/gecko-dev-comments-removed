







































#ifndef __MAI_INTERFACE_DOCUMENT_H__
#define __MAI_INTERFACE_DOCUMENT_H__

#include "nsMai.h"
#include "nsIAccessibleDocument.h"

G_BEGIN_DECLS


void documentInterfaceInitCB(AtkDocumentIface *aIface);
AtkAttributeSet* getDocumentAttributesCB(AtkDocument *aDocument);
const gchar* getDocumentAttributeValueCB(AtkDocument *aDocument,
                                         const gchar *aAttrName);

G_END_DECLS

#endif 
