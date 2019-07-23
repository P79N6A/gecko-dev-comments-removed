







































#ifndef _ACCESSIBLE_TABLECELL_H
#define _ACCESSIBLE_TABLECELL_H

#include "nsISupports.h"

#include "AccessibleTableCell.h"

class CAccessibleTableCell: public nsISupports,
                            public IAccessibleTableCell
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  

  virtual  HRESULT STDMETHODCALLTYPE get_table(
       IUnknown **table);

  virtual  HRESULT STDMETHODCALLTYPE get_columnExtent(
       long *nColumnsSpanned);

  virtual  HRESULT STDMETHODCALLTYPE get_columnHeaderCells(
       IUnknown ***cellAccessibles,
       long *nColumnHeaderCells);

  virtual  HRESULT STDMETHODCALLTYPE get_columnIndex(
       long *columnIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_rowExtent(
       long *nRowsSpanned);

  virtual  HRESULT STDMETHODCALLTYPE get_rowHeaderCells(
       IUnknown ***cellAccessibles,
       long *nRowHeaderCells);

  virtual  HRESULT STDMETHODCALLTYPE get_rowIndex(
       long *rowIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_rowColumnExtents(
       long *row,
       long *column,
       long *rowExtents,
       long *columnExtents,
       boolean *isSelected);

  virtual  HRESULT STDMETHODCALLTYPE get_isSelected(
       boolean *isSelected);
};

#endif
