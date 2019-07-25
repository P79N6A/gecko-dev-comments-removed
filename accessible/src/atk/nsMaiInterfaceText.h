






































#ifndef __MAI_INTERFACE_TEXT_H__
#define __MAI_INTERFACE_TEXT_H__

#include "nsMai.h"
#include "nsIAccessibleText.h"

G_BEGIN_DECLS

void textInterfaceInitCB(AtkTextIface *aIface);


gchar *getTextCB(AtkText *aText,
                 gint aStartOffset, gint aEndOffset);
gchar *getTextAfterOffsetCB(AtkText *aText, gint aOffset,
                            AtkTextBoundary aBoundaryType,
                            gint *aStartOffset, gint *aEndOffset);
gchar *getTextAtOffsetCB(AtkText *aText, gint aOffset,
                         AtkTextBoundary aBoundaryType,
                         gint *aStartOffset, gint *aEndOffset);
gunichar getCharacterAtOffsetCB(AtkText *aText, gint aOffset);
gchar *getTextBeforeOffsetCB(AtkText *aText, gint aOffset,
                             AtkTextBoundary aBoundaryType,
                             gint *aStartOffset, gint *aEndOffset);
gint getCaretOffsetCB(AtkText *aText);
AtkAttributeSet *getRunAttributesCB(AtkText *aText, gint aOffset,
                                    gint *aStartOffset,
                                    gint *aEndOffset);
AtkAttributeSet* getDefaultAttributesCB(AtkText *aText);
void getCharacterExtentsCB(AtkText *aText, gint aOffset,
                           gint *aX, gint *aY,
                           gint *aWidth, gint *aHeight,
                           AtkCoordType aCoords);
void getRangeExtentsCB(AtkText *aText, gint aStartOffset,
                       gint aEndOffset, AtkCoordType aCoords,
                       AtkTextRectangle *aRect);
gint getCharacterCountCB(AtkText *aText);
gint getOffsetAtPointCB(AtkText *aText,
                        gint aX, gint aY,
                        AtkCoordType aCoords);
gint getTextSelectionCountCB(AtkText *aText);
gchar *getTextSelectionCB(AtkText *aText, gint aSelectionNum,
                          gint *aStartOffset, gint *aEndOffset);


gboolean addTextSelectionCB(AtkText *aText,
                            gint aStartOffset,
                            gint aEndOffset);
gboolean removeTextSelectionCB(AtkText *aText,
                               gint aSelectionNum);
gboolean setTextSelectionCB(AtkText *aText, gint aSelectionNum,
                            gint aStartOffset, gint aEndOffset);
gboolean setCaretOffsetCB(AtkText *aText, gint aOffset);








G_END_DECLS

#endif 
