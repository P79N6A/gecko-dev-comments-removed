





#ifndef TABLE_ACCESSIBLE_H
#define TABLE_ACCESSIBLE_H

#include "nsString.h"
#include "nsTArray.h"
#include "prtypes.h"

class Accessible;

namespace mozilla {
namespace a11y {




class TableAccessible
{
public:

  


  virtual Accessible* Caption() { return nsnull; }

  


  virtual void Summary(nsString& aSummary) { aSummary.Truncate(); }

  


  virtual PRUint32 ColCount() { return 0; }

  


  virtual PRUint32 RowCount() { return 0; }

  


  virtual Accessible* CellAt(PRUint32 aRowIdx, PRUint32 aColIdx) { return nsnull; }

  


  virtual PRInt32 CellIndexAt(PRUint32 aRowIdx, PRUint32 aColIdx)
    { return ColCount() * aRowIdx + aColIdx; }

  


  virtual PRInt32 ColIndexAt(PRUint32 aCellIdx) { return -1; }

  


  virtual PRInt32 RowIndexAt(PRUint32 aCellIdx) { return -1; }

  


  virtual void RowAndColIndicesAt(PRUint32 aCellIdx, PRInt32* aRowIdx,
                                  PRInt32* aColIdx) {}

  



  virtual PRUint32 ColExtentAt(PRUint32 aRowIdx, PRUint32 aColIdx) { return 1; }

  



  virtual PRUint32 RowExtentAt(PRUint32 aRowIdx, PRUint32 aColIdx) { return 1; }

  


  virtual void ColDescription(PRUint32 aColIdx, nsString& aDescription)
    { aDescription.Truncate(); }

  


  virtual void RowDescription(PRUint32 aRowIdx, nsString& aDescription)
    { aDescription.Truncate(); }

  


  virtual bool IsColSelected(PRUint32 aColIdx) { return false; }

  


  virtual bool IsRowSelected(PRUint32 aRowIdx) { return false; }

  


  virtual bool IsCellSelected(PRUint32 aRowIdx, PRUint32 aColIdx) { return false; }

  


  virtual PRUint32 SelectedCellCount() { return 0; }

  


  virtual PRUint32 SelectedColCount() { return 0; }

  


  virtual PRUint32 SelectedRowCount() { return 0; }

  


  virtual void SelectedCells(nsTArray<Accessible*>* aCells) {}

  


  virtual void SelectedCellIndices(nsTArray<PRUint32>* aCells) = 0;

  


  virtual void SelectedColIndices(nsTArray<PRUint32>* aCols) = 0;

  


  virtual void SelectedRowIndices(nsTArray<PRUint32>* aRows) = 0;

  


  virtual void SelectCol(PRUint32 aColIdx) {}

  


  virtual void SelectRow(PRUint32 aRowIdx) {}

  


  virtual void UnselectCol(PRUint32 aColIdx) {}

  


  virtual void UnselectRow(PRUint32 aRowIdx) {}

  


  virtual bool IsProbablyLayoutTable() { return false; }
};

} 
} 

#endif
