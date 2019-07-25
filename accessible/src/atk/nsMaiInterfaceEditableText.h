






































#ifndef __MAI_INTERFACE_EDITABLETEXT_H__
#define __MAI_INTERFACE_EDITABLETEXT_H__

#include "nsMai.h"
#include "nsIAccessibleEditableText.h"

G_BEGIN_DECLS

void editableTextInterfaceInitCB(AtkEditableTextIface *aIface);


gboolean setRunAttributesCB(AtkEditableText *aText,
                            AtkAttributeSet *aAttribSet,
                            gint aStartOffset,
                            gint aEndOffset);
void setTextContentsCB(AtkEditableText *aText, const gchar *aString);
void insertTextCB(AtkEditableText *aText,
                  const gchar *aString, gint aLength, gint *aPosition);
void copyTextCB(AtkEditableText *aText, gint aStartPos, gint aEndPos);
void cutTextCB(AtkEditableText *aText, gint aStartPos, gint aEndPos);
void deleteTextCB(AtkEditableText *aText, gint aStartPos, gint aEndPos);
void pasteTextCB(AtkEditableText *aText, gint aPosition);

G_END_DECLS

#endif 
