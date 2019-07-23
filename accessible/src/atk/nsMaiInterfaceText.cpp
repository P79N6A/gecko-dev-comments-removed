








































#include "nsMaiInterfaceText.h"
#include "nsString.h"

AtkAttributeSet * GetAttributeSet(nsIAccessible* aAccessible);

void
textInterfaceInitCB(AtkTextIface *aIface)
{
    NS_ASSERTION(aIface, "Invalid aIface");
    if (!aIface)
        return;

    aIface->get_text = getTextCB;
    aIface->get_text_after_offset = getTextAfterOffsetCB;
    aIface->get_text_at_offset = getTextAtOffsetCB;
    aIface->get_character_at_offset = getCharacterAtOffsetCB;
    aIface->get_text_before_offset = getTextBeforeOffsetCB;
    aIface->get_caret_offset = getCaretOffsetCB;
    aIface->get_run_attributes = getRunAttributesCB;
    aIface->get_default_attributes = getDefaultAttributesCB;
    aIface->get_character_extents = getCharacterExtentsCB;
    aIface->get_range_extents = getRangeExtentsCB;
    aIface->get_character_count = getCharacterCountCB;
    aIface->get_offset_at_point = getOffsetAtPointCB;
    aIface->get_n_selections = getTextSelectionCountCB;
    aIface->get_selection = getTextSelectionCB;

    
    aIface->add_selection = addTextSelectionCB;
    aIface->remove_selection = removeTextSelectionCB;
    aIface->set_selection = setTextSelectionCB;
    aIface->set_caret_offset = setCaretOffsetCB;
}


void ConvertTexttoAsterisks(nsAccessibleWrap* accWrap, nsAString& aString)
{
    
    PRUint32 accRole;
    accWrap->GetRole(&accRole);
    if (NS_STATIC_CAST(AtkRole, accRole) == ATK_ROLE_PASSWORD_TEXT) {
        for (PRUint32 i = 0; i < aString.Length(); i++)
            aString.Replace(i, 1, NS_LITERAL_STRING("*"));
    }
}

gchar *
getTextCB(AtkText *aText, gint aStartOffset, gint aEndOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, nsnull);

    nsAutoString autoStr;
    nsresult rv = accText->GetText(aStartOffset, aEndOffset, autoStr);
    NS_ENSURE_SUCCESS(rv, nsnull);

    ConvertTexttoAsterisks(accWrap, autoStr);
    NS_ConvertUTF16toUTF8 cautoStr(autoStr);

    
    return (cautoStr.get()) ? g_strdup(cautoStr.get()) : nsnull;
}

gchar *
getTextAfterOffsetCB(AtkText *aText, gint aOffset,
                     AtkTextBoundary aBoundaryType,
                     gint *aStartOffset, gint *aEndOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, nsnull);

    nsAutoString autoStr;
    PRInt32 startOffset = 0, endOffset = 0;
    nsresult rv =
        accText->GetTextAfterOffset(aOffset, aBoundaryType,
                                    &startOffset, &endOffset, autoStr);
    *aStartOffset = startOffset;
    *aEndOffset = endOffset;

    NS_ENSURE_SUCCESS(rv, nsnull);

    ConvertTexttoAsterisks(accWrap, autoStr);
    NS_ConvertUTF16toUTF8 cautoStr(autoStr);
    return (cautoStr.get()) ? g_strdup(cautoStr.get()) : nsnull;
}

gchar *
getTextAtOffsetCB(AtkText *aText, gint aOffset,
                  AtkTextBoundary aBoundaryType,
                  gint *aStartOffset, gint *aEndOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, nsnull);

    nsAutoString autoStr;
    PRInt32 startOffset = 0, endOffset = 0;
    nsresult rv =
        accText->GetTextAtOffset(aOffset, aBoundaryType,
                                 &startOffset, &endOffset, autoStr);
    *aStartOffset = startOffset;
    *aEndOffset = endOffset;

    NS_ENSURE_SUCCESS(rv, nsnull);

    ConvertTexttoAsterisks(accWrap, autoStr);
    NS_ConvertUTF16toUTF8 cautoStr(autoStr);
    return (cautoStr.get()) ? g_strdup(cautoStr.get()) : nsnull;
}

gunichar
getCharacterAtOffsetCB(AtkText *aText, gint aOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return 0;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, 0);

    
    
    PRUnichar uniChar;
    nsresult rv =
        accText->GetCharacterAtOffset(aOffset, &uniChar);

    
    PRUint32 accRole;
    accWrap->GetRole(&accRole);
    if (NS_STATIC_CAST(AtkRole, accRole) == ATK_ROLE_PASSWORD_TEXT) {
        uniChar = '*';
    }

    return (NS_FAILED(rv)) ? 0 : NS_STATIC_CAST(gunichar, uniChar);
}

gchar *
getTextBeforeOffsetCB(AtkText *aText, gint aOffset,
                      AtkTextBoundary aBoundaryType,
                      gint *aStartOffset, gint *aEndOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, nsnull);

    nsAutoString autoStr;
    PRInt32 startOffset = 0, endOffset = 0;
    nsresult rv =
        accText->GetTextBeforeOffset(aOffset, aBoundaryType,
                                     &startOffset, &endOffset, autoStr);
    *aStartOffset = startOffset;
    *aEndOffset = endOffset;

    NS_ENSURE_SUCCESS(rv, nsnull);

    ConvertTexttoAsterisks(accWrap, autoStr);
    NS_ConvertUTF16toUTF8 cautoStr(autoStr);
    return (cautoStr.get()) ? g_strdup(cautoStr.get()) : nsnull;
}

gint
getCaretOffsetCB(AtkText *aText)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return 0;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, 0);

    PRInt32 offset;
    nsresult rv = accText->GetCaretOffset(&offset);
    return (NS_FAILED(rv)) ? 0 : NS_STATIC_CAST(gint, offset);
}

AtkAttributeSet *
getRunAttributesCB(AtkText *aText, gint aOffset,
                   gint *aStartOffset,
                   gint *aEndOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, nsnull);

    nsCOMPtr<nsIAccessible> accessibleWithAttrs;
    PRInt32 startOffset = 0, endOffset = 0;
    nsresult rv = accText->GetAttributeRange(aOffset,
                                             &startOffset, &endOffset,
                                             getter_AddRefs(accessibleWithAttrs));
    *aStartOffset = startOffset;
    *aEndOffset = endOffset;
    NS_ENSURE_SUCCESS(rv, nsnull);

    return GetAttributeSet(accessibleWithAttrs);
}

AtkAttributeSet *
getDefaultAttributesCB(AtkText *aText)
{
    
    return nsnull;
}

void
getCharacterExtentsCB(AtkText *aText, gint aOffset,
                      gint *aX, gint *aY,
                      gint *aWidth, gint *aHeight,
                      AtkCoordType aCoords)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if(!accWrap || !aX || !aY || !aWidth || !aHeight)
        return;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    if (!accText)
        return;

    PRInt32 extY = 0, extX = 0;
    PRInt32 extWidth = 0, extHeight = 0;

    PRUint32 geckoCoordType;
    if (aCoords == ATK_XY_SCREEN)
        geckoCoordType = nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE;
    else
        geckoCoordType = nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE;

    nsresult rv = accText->GetCharacterExtents(aOffset, &extX, &extY,
                                               &extWidth, &extHeight,
                                               geckoCoordType);
    *aX = extX;
    *aY = extY;
    *aWidth = extWidth;
    *aHeight = extHeight;
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "MaiInterfaceText::GetCharacterExtents, failed\n");
}

void
getRangeExtentsCB(AtkText *aText, gint aStartOffset, gint aEndOffset,
                  AtkCoordType aCoords, AtkTextRectangle *aRect)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if(!accWrap || !aRect)
        return;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    if (!accText)
        return;

    PRInt32 extY = 0, extX = 0;
    PRInt32 extWidth = 0, extHeight = 0;

    PRUint32 geckoCoordType;
    if (aCoords == ATK_XY_SCREEN)
        geckoCoordType = nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE;
    else
        geckoCoordType = nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE;

    nsresult rv = accText->GetRangeExtents(aStartOffset, aEndOffset,
                                           &extX, &extY,
                                           &extWidth, &extHeight,
                                           geckoCoordType);
    aRect->x = extX;
    aRect->y = extY;
    aRect->width = extWidth;
    aRect->height = extHeight;
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "MaiInterfaceText::GetRangeExtents, failed\n");
}

gint
getCharacterCountCB(AtkText *aText)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return 0;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, 0);

    PRInt32 count = 0;
    nsresult rv = accText->GetCharacterCount(&count);
    return (NS_FAILED(rv)) ? 0 : NS_STATIC_CAST(gint, count);
}

gint
getOffsetAtPointCB(AtkText *aText,
                   gint aX, gint aY,
                   AtkCoordType aCoords)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, -1);

    PRInt32 offset = 0;
    PRUint32 geckoCoordType;
    if (aCoords == ATK_XY_SCREEN)
        geckoCoordType = nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE;
    else
        geckoCoordType = nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE;

    accText->GetOffsetAtPoint(aX, aY, geckoCoordType, &offset);
    return NS_STATIC_CAST(gint, offset);
}

gint
getTextSelectionCountCB(AtkText *aText)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));

    PRInt32 selectionCount;
    nsresult rv = accText->GetSelectionCount(&selectionCount);
 
    return NS_FAILED(rv) ? 0 : selectionCount;
}

gchar *
getTextSelectionCB(AtkText *aText, gint aSelectionNum,
                   gint *aStartOffset, gint *aEndOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, nsnull);

    PRInt32 startOffset = 0, endOffset = 0;
    nsresult rv = accText->GetSelectionBounds(aSelectionNum,
                                              &startOffset, &endOffset);

    *aStartOffset = startOffset;
    *aEndOffset = endOffset;

    NS_ENSURE_SUCCESS(rv, nsnull);

    return getTextCB(aText, *aStartOffset, *aEndOffset);
}


gboolean
addTextSelectionCB(AtkText *aText,
                   gint aStartOffset,
                   gint aEndOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, FALSE);

    nsresult rv = accText->AddSelection(aStartOffset, aEndOffset);

    return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

gboolean
removeTextSelectionCB(AtkText *aText,
                      gint aSelectionNum)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, FALSE);

    nsresult rv = accText->RemoveSelection(aSelectionNum);

    return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

gboolean
setTextSelectionCB(AtkText *aText, gint aSelectionNum,
                   gint aStartOffset, gint aEndOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, FALSE);

    nsresult rv = accText->SetSelectionBounds(aSelectionNum,
                                              aStartOffset, aEndOffset);
    return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}

gboolean
setCaretOffsetCB(AtkText *aText, gint aOffset)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aText));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleText> accText;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleText),
                            getter_AddRefs(accText));
    NS_ENSURE_TRUE(accText, FALSE);

    nsresult rv = accText->SetCaretOffset(aOffset);
    return NS_SUCCEEDED(rv) ? TRUE : FALSE;
}
