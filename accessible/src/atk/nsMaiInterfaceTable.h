







































#ifndef __MAI_INTERFACE_TABLE_H__
#define __MAI_INTERFACE_TABLE_H__

#include "nsMai.h"
#include "nsIAccessibleTable.h"

G_BEGIN_DECLS


void tableInterfaceInitCB(AtkTableIface *aIface);
AtkObject* refAtCB(AtkTable *aTable, gint aRow, gint aColumn);
gint getIndexAtCB(AtkTable *aTable, gint aRow, gint aColumn);
gint getColumnAtIndexCB(AtkTable *aTable, gint aIndex);
gint getRowAtIndexCB(AtkTable *aTable, gint aIndex);
gint getColumnCountCB(AtkTable *aTable);
gint getRowCountCB(AtkTable *aTable);
gint getColumnExtentAtCB(AtkTable *aTable, gint aRow, gint aColumn);
gint getRowExtentAtCB(AtkTable *aTable, gint aRow, gint aColumn);
AtkObject* getCaptionCB(AtkTable *aTable);
const gchar* getColumnDescriptionCB(AtkTable *aTable, gint aColumn);
AtkObject* getColumnHeaderCB(AtkTable *aTable, gint aColumn);
const gchar* getRowDescriptionCB(AtkTable *aTable, gint aRow);
AtkObject* getRowHeaderCB(AtkTable *aTable, gint aRow);
AtkObject* getSummaryCB(AtkTable *aTable);
gint getSelectedColumnsCB(AtkTable *aTable, gint **aSelected);
gint getSelectedRowsCB(AtkTable *aTable, gint **aSelected);
gboolean isColumnSelectedCB(AtkTable *aTable, gint aColumn);
gboolean isRowSelectedCB(AtkTable *aTable, gint aRow);
gboolean isCellSelectedCB(AtkTable *aTable, gint aRow, gint aColumn);


















































G_END_DECLS

#endif 
