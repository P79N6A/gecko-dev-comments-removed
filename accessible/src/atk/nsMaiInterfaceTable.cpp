







































#include "nsMaiInterfaceTable.h"

#include "nsArrayUtils.h"

void
tableInterfaceInitCB(AtkTableIface *aIface)

{
    g_return_if_fail(aIface != NULL);

    aIface->ref_at = refAtCB;
    aIface->get_index_at = getIndexAtCB;
    aIface->get_column_at_index = getColumnAtIndexCB;
    aIface->get_row_at_index = getRowAtIndexCB;
    aIface->get_n_columns = getColumnCountCB;
    aIface->get_n_rows = getRowCountCB;
    aIface->get_column_extent_at = getColumnExtentAtCB;
    aIface->get_row_extent_at = getRowExtentAtCB;
    aIface->get_caption = getCaptionCB;
    aIface->get_column_description = getColumnDescriptionCB;
    aIface->get_column_header = getColumnHeaderCB;
    aIface->get_row_description = getRowDescriptionCB;
    aIface->get_row_header = getRowHeaderCB;
    aIface->get_summary = getSummaryCB;
    aIface->get_selected_columns = getSelectedColumnsCB;
    aIface->get_selected_rows = getSelectedRowsCB;
    aIface->is_column_selected = isColumnSelectedCB;
    aIface->is_row_selected = isRowSelectedCB;
    aIface->is_selected = isCellSelectedCB;
}


AtkObject*
refAtCB(AtkTable *aTable, gint aRow, gint aColumn)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, nsnull);

    nsCOMPtr<nsIAccessible> cell;
    nsresult rv = accTable->GetCellAt(aRow, aColumn,getter_AddRefs(cell));
    if (NS_FAILED(rv) || !cell)
        return nsnull;

    AtkObject *cellAtkObj = nsAccessibleWrap::GetAtkObject(cell);
    if (cellAtkObj) {
        g_object_ref(cellAtkObj);
    }
    return cellAtkObj;
}

gint
getIndexAtCB(AtkTable *aTable, gint aRow, gint aColumn)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, -1);

    PRInt32 index;
    nsresult rv = accTable->GetCellIndexAt(aRow, aColumn, &index);
    NS_ENSURE_SUCCESS(rv, -1);

    return static_cast<gint>(index);
}

gint
getColumnAtIndexCB(AtkTable *aTable, gint aIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, -1);

    PRInt32 col;
    nsresult rv = accTable->GetColumnIndexAt(aIndex, &col);
    NS_ENSURE_SUCCESS(rv, -1);

    return static_cast<gint>(col);
}

gint
getRowAtIndexCB(AtkTable *aTable, gint aIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, -1);

    PRInt32 row;
    nsresult rv = accTable->GetRowIndexAt(aIndex, &row);
    NS_ENSURE_SUCCESS(rv, -1);

    return static_cast<gint>(row);
}

gint
getColumnCountCB(AtkTable *aTable)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, -1);

    PRInt32 count;
    nsresult rv = accTable->GetColumnCount(&count);
    NS_ENSURE_SUCCESS(rv, -1);

    return static_cast<gint>(count);
}

gint
getRowCountCB(AtkTable *aTable)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, -1);

    PRInt32 count;
    nsresult rv = accTable->GetRowCount(&count);
    NS_ENSURE_SUCCESS(rv, -1);

    return static_cast<gint>(count);
}

gint
getColumnExtentAtCB(AtkTable *aTable,
                    gint aRow, gint aColumn)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, -1);

    PRInt32 extent;
    nsresult rv = accTable->GetColumnExtentAt(aRow, aColumn, &extent);
    NS_ENSURE_SUCCESS(rv, -1);

    return static_cast<gint>(extent);
}

gint
getRowExtentAtCB(AtkTable *aTable,
                 gint aRow, gint aColumn)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, -1);

    PRInt32 extent;
    nsresult rv = accTable->GetRowExtentAt(aRow, aColumn, &extent);
    NS_ENSURE_SUCCESS(rv, -1);

    return static_cast<gint>(extent);
}

AtkObject*
getCaptionCB(AtkTable *aTable)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, nsnull);

    nsCOMPtr<nsIAccessible> caption;
    nsresult rv = accTable->GetCaption(getter_AddRefs(caption));
    if (NS_FAILED(rv) || !caption)
        return nsnull;

    return nsAccessibleWrap::GetAtkObject(caption);
}

const gchar*
getColumnDescriptionCB(AtkTable *aTable, gint aColumn)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, nsnull);

    nsAutoString autoStr;
    nsresult rv = accTable->GetColumnDescription(aColumn, autoStr);
    NS_ENSURE_SUCCESS(rv, nsnull);

    return nsAccessibleWrap::ReturnString(autoStr);
}

AtkObject*
getColumnHeaderCB(AtkTable *aTable, gint aColumn)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, nsnull);

    nsCOMPtr<nsIAccessible> accCell;
    accTable->GetCellAt(0, aColumn, getter_AddRefs(accCell));
    if (!accCell)
        return nsnull;

    
    
    if (nsAccUtils::Role(accCell) == nsIAccessibleRole::ROLE_COLUMNHEADER)
        return nsAccessibleWrap::GetAtkObject(accCell);

    
    nsCOMPtr<nsIAccessibleTableCell> accTableCell =
        do_QueryInterface(accCell);

    if (accTableCell) {
        nsCOMPtr<nsIArray> headerCells;
        accTableCell->GetColumnHeaderCells(getter_AddRefs(headerCells));
        if (headerCells) {
            nsresult rv;
            nsCOMPtr<nsIAccessible> accHeaderCell =
                do_QueryElementAt(headerCells, 0, &rv);
            NS_ENSURE_SUCCESS(rv, nsnull);

            return nsAccessibleWrap::GetAtkObject(accHeaderCell);
        }
    }

    return nsnull;
}

const gchar*
getRowDescriptionCB(AtkTable *aTable, gint aRow)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, nsnull);

    nsAutoString autoStr;
    nsresult rv = accTable->GetRowDescription(aRow, autoStr);
    NS_ENSURE_SUCCESS(rv, nsnull);

    return nsAccessibleWrap::ReturnString(autoStr);
}

AtkObject*
getRowHeaderCB(AtkTable *aTable, gint aRow)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, nsnull);

    nsCOMPtr<nsIAccessible> accCell;
    accTable->GetCellAt(aRow, 0, getter_AddRefs(accCell));
    if (!accCell)
      return nsnull;

    
    
    if (nsAccUtils::Role(accCell) == nsIAccessibleRole::ROLE_ROWHEADER)
        return nsAccessibleWrap::GetAtkObject(accCell);

    
    nsCOMPtr<nsIAccessibleTableCell> accTableCell =
        do_QueryInterface(accCell);

    if (accTableCell) {
      nsCOMPtr<nsIArray> headerCells;
      accTableCell->GetRowHeaderCells(getter_AddRefs(headerCells));
      if (headerCells) {
        nsresult rv;
        nsCOMPtr<nsIAccessible> accHeaderCell =
            do_QueryElementAt(headerCells, 0, &rv);
        NS_ENSURE_SUCCESS(rv, nsnull);

        return nsAccessibleWrap::GetAtkObject(accHeaderCell);
      }
    }

    return nsnull;
}

AtkObject*
getSummaryCB(AtkTable *aTable)
{
    
    
    
    
    return nsnull;
}

gint
getSelectedColumnsCB(AtkTable *aTable, gint **aSelected)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return 0;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, 0);

    PRUint32 size = 0;
    PRInt32 *columns = NULL;
    nsresult rv = accTable->GetSelectedColumnIndices(&size, &columns);
    if (NS_FAILED(rv) || (size == 0) || !columns) {
        *aSelected = nsnull;
        return 0;
    }

    gint *atkColumns = g_new(gint, size);
    if (!atkColumns) {
        NS_WARNING("OUT OF MEMORY");
        return nsnull;
    }

    
    for (PRUint32 index = 0; index < size; ++index)
        atkColumns[index] = static_cast<gint>(columns[index]);
    nsMemory::Free(columns);

    *aSelected = atkColumns;
    return size;
}

gint
getSelectedRowsCB(AtkTable *aTable, gint **aSelected)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return 0;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, 0);

    PRUint32 size = 0;
    PRInt32 *rows = NULL;
    nsresult rv = accTable->GetSelectedRowIndices(&size, &rows);
    if (NS_FAILED(rv) || (size == 0) || !rows) {
        *aSelected = nsnull;
        return 0;
    }

    gint *atkRows = g_new(gint, size);
    if (!atkRows) {
        NS_WARNING("OUT OF MEMORY");
        return nsnull;
    }

    
    for (PRUint32 index = 0; index < size; ++index)
        atkRows[index] = static_cast<gint>(rows[index]);
    nsMemory::Free(rows);

    *aSelected = atkRows;
    return size;
}

gboolean
isColumnSelectedCB(AtkTable *aTable, gint aColumn)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, FALSE);

    PRBool outValue;
    nsresult rv = accTable->IsColumnSelected(aColumn, &outValue);
    return NS_FAILED(rv) ? FALSE : static_cast<gboolean>(outValue);
}

gboolean
isRowSelectedCB(AtkTable *aTable, gint aRow)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, FALSE);

    PRBool outValue;
    nsresult rv = accTable->IsRowSelected(aRow, &outValue);
    return NS_FAILED(rv) ? FALSE : static_cast<gboolean>(outValue);
}

gboolean
isCellSelectedCB(AtkTable *aTable, gint aRow, gint aColumn)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aTable));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleTable> accTable;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleTable),
                            getter_AddRefs(accTable));
    NS_ENSURE_TRUE(accTable, FALSE);

    PRBool outValue;
    nsresult rv = accTable->IsCellSelected(aRow, aColumn, &outValue);
    return NS_FAILED(rv) ? FALSE : static_cast<gboolean>(outValue);
}
