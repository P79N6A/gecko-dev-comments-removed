







































#include "nsString.h"
#include "nsMaiInterfaceEditableText.h"

void
editableTextInterfaceInitCB(AtkEditableTextIface *aIface)
{
    NS_ASSERTION(aIface, "Invalid aIface");
    if (!aIface)
        return;

    aIface->set_run_attributes = setRunAttributesCB;
    aIface->set_text_contents = setTextContentsCB;
    aIface->insert_text = insertTextCB;
    aIface->copy_text = copyTextCB;
    aIface->cut_text = cutTextCB;
    aIface->delete_text = deleteTextCB;
    aIface->paste_text = pasteTextCB;
}



gboolean
setRunAttributesCB(AtkEditableText *aText, AtkAttributeSet *aAttribSet,
                   gint aStartOffset, gint aEndOffset)

{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleEditableText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleEditableText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, FALSE);

    nsCOMPtr<nsISupports> attrSet;
    

    nsresult rv = accText->SetAttributes(aStartOffset, aEndOffset,
                                         attrSet);
    return NS_FAILED(rv) ? FALSE : TRUE;
}

void
setTextContentsCB(AtkEditableText *aText, const gchar *aString)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return;

    nsCOMPtr<nsIAccessibleEditableText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleEditableText),
                            getter_AddRefs(accText));
    if (!accText)
        return;

    MAI_LOG_DEBUG(("EditableText: setTextContentsCB, aString=%s", aString));

    NS_ConvertUTF8toUTF16 strContent(aString);
    accText->SetTextContents(strContent);
}

void
insertTextCB(AtkEditableText *aText,
             const gchar *aString, gint aLength, gint *aPosition)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return;

    nsCOMPtr<nsIAccessibleEditableText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleEditableText),
                            getter_AddRefs(accText));
    if (!accText)
        return;

    NS_ConvertUTF8toUTF16 strContent(aString);

    
    
    
    
    

    accText->InsertText(strContent, *aPosition);

    MAI_LOG_DEBUG(("EditableText: insert aString=%s, aLength=%d, aPosition=%d",
                   aString, aLength, *aPosition));
}

void
copyTextCB(AtkEditableText *aText, gint aStartPos, gint aEndPos)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return;

    nsCOMPtr<nsIAccessibleEditableText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleEditableText),
                            getter_AddRefs(accText));
    if (!accText)
        return;

    MAI_LOG_DEBUG(("EditableText: copyTextCB, aStartPos=%d, aEndPos=%d",
                   aStartPos, aEndPos));
    accText->CopyText(aStartPos, aEndPos);
}

void
cutTextCB(AtkEditableText *aText, gint aStartPos, gint aEndPos)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return;

    nsCOMPtr<nsIAccessibleEditableText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleEditableText),
                            getter_AddRefs(accText));
    if (!accText)
        return;
    MAI_LOG_DEBUG(("EditableText: cutTextCB, aStartPos=%d, aEndPos=%d",
                   aStartPos, aEndPos));
    accText->CutText(aStartPos, aEndPos);
}

void
deleteTextCB(AtkEditableText *aText, gint aStartPos, gint aEndPos)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return;

    nsCOMPtr<nsIAccessibleEditableText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleEditableText),
                            getter_AddRefs(accText));
    if (!accText)
        return;

    MAI_LOG_DEBUG(("EditableText: deleteTextCB, aStartPos=%d, aEndPos=%d",
                   aStartPos, aEndPos));
    accText->DeleteText(aStartPos, aEndPos);
}

void
pasteTextCB(AtkEditableText *aText, gint aPosition)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return;

    nsCOMPtr<nsIAccessibleEditableText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleEditableText),
                            getter_AddRefs(accText));
    if (!accText)
        return;

    MAI_LOG_DEBUG(("EditableText: pasteTextCB, aPosition=%d", aPosition));
    accText->PasteText(aPosition);
}
