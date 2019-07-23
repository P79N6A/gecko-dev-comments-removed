







































#ifndef _ACCESSIBLE_TABLE_H
#define _ACCESSIBLE_TABLE_H

#include "nsISupports.h"

#include "AccessibleTable.h"
#include "AccessibleTable2.h"

class CAccessibleTable: public nsISupports,
                        public IAccessibleTable,
                        public IAccessibleTable2
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual  HRESULT STDMETHODCALLTYPE get_accessibleAt(
       long row,
       long column,
       IUnknown **accessible);

  virtual  HRESULT STDMETHODCALLTYPE get_caption(
       IUnknown **accessible);

  virtual  HRESULT STDMETHODCALLTYPE get_childIndex(
       long rowIndex,
       long columnIndex,
       long *childIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_columnDescription(
       long column,
       BSTR *description);

  virtual  HRESULT STDMETHODCALLTYPE get_columnExtentAt( 
       long row,
       long column,
       long *nColumnsSpanned);

  virtual  HRESULT STDMETHODCALLTYPE get_columnHeader(
       IAccessibleTable **accessibleTable,
       long *startingRowIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_columnIndex(
       long childIndex,
       long *columnIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_nColumns(
       long *columnCount);

  virtual  HRESULT STDMETHODCALLTYPE get_nRows(
       long *rowCount);

  virtual  HRESULT STDMETHODCALLTYPE get_nSelectedChildren(
       long *childCount);

  virtual  HRESULT STDMETHODCALLTYPE get_nSelectedColumns(
       long *columnCount);

  virtual  HRESULT STDMETHODCALLTYPE get_nSelectedRows(
       long *rowCount);

  virtual  HRESULT STDMETHODCALLTYPE get_rowDescription(
       long row,
       BSTR *description);

  virtual  HRESULT STDMETHODCALLTYPE get_rowExtentAt(
       long row,
       long column,
       long *nRowsSpanned);

  virtual  HRESULT STDMETHODCALLTYPE get_rowHeader(
       IAccessibleTable **accessibleTable,
       long *startingColumnIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_rowIndex(
       long childIndex,
       long *rowIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_selectedChildren(
       long maxChildren,
       long **children,
       long *nChildren);

  virtual  HRESULT STDMETHODCALLTYPE get_selectedColumns(
       long maxColumns,
       long **columns,
       long *nColumns);

  virtual  HRESULT STDMETHODCALLTYPE get_selectedRows(
       long maxRows,
       long **rows,
       long *nRows);

  virtual  HRESULT STDMETHODCALLTYPE get_summary(
       IUnknown **accessible);

  virtual  HRESULT STDMETHODCALLTYPE get_isColumnSelected(
       long column,
       boolean *isSelected);

  virtual  HRESULT STDMETHODCALLTYPE get_isRowSelected(
       long row,
       boolean *isSelected);

  virtual  HRESULT STDMETHODCALLTYPE get_isSelected(
       long row,
       long column,
       boolean *isSelected);

  virtual HRESULT STDMETHODCALLTYPE selectRow(
       long row);

  virtual HRESULT STDMETHODCALLTYPE selectColumn(
       long column);

  virtual HRESULT STDMETHODCALLTYPE unselectRow(
       long row);

  virtual HRESULT STDMETHODCALLTYPE unselectColumn(
       long column);

  virtual  HRESULT STDMETHODCALLTYPE get_rowColumnExtentsAtIndex(
       long index,
       long *row,
       long *column,
       long *rowExtents,
       long *columnExtents,
       boolean *isSelected);

  virtual  HRESULT STDMETHODCALLTYPE get_modelChange(
       IA2TableModelChange *modelChange);


  

  virtual  HRESULT STDMETHODCALLTYPE get_cellAt(
       long row,
       long column,
       IUnknown **cell);

  virtual  HRESULT STDMETHODCALLTYPE get_nSelectedCells(
       long *cellCount);

  virtual  HRESULT STDMETHODCALLTYPE get_selectedCells(
       IUnknown ***cells,
       long *nSelectedCells);

  virtual  HRESULT STDMETHODCALLTYPE get_selectedColumns(
       long **selectedColumns,
       long *nColumns);

  virtual  HRESULT STDMETHODCALLTYPE get_selectedRows(
       long **selectedRows, 
       long *nRows);

private:
  enum eItemsType {
    ITEMSTYPE_CELLS,
    ITEMSTYPE_COLUMNS,
    ITEMSTYPE_ROWS
  };

  HRESULT GetSelectedItems(long **aItems, long *aItemsCount, eItemsType aType);
};

#endif
