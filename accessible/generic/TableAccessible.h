





#ifndef TABLE_ACCESSIBLE_H
#define TABLE_ACCESSIBLE_H

#include "nsString.h"
#include "nsTArray.h"

namespace mozilla {
namespace a11y {

class Accessible;




class TableAccessible
{
public:

  


  virtual Accessible* Caption() const { return nullptr; }

  


  virtual void Summary(nsString& aSummary) { aSummary.Truncate(); }

  


  virtual uint32_t ColCount() { return 0; }

  


  virtual uint32_t RowCount() { return 0; }

  


  virtual Accessible* CellAt(uint32_t aRowIdx, uint32_t aColIdx) { return nullptr; }

  


  virtual int32_t CellIndexAt(uint32_t aRowIdx, uint32_t aColIdx)
    { return ColCount() * aRowIdx + aColIdx; }

  


  virtual int32_t ColIndexAt(uint32_t aCellIdx) 
    { return aCellIdx % ColCount(); }

  


  virtual int32_t RowIndexAt(uint32_t aCellIdx) 
    { return aCellIdx / ColCount(); }

  


  virtual void RowAndColIndicesAt(uint32_t aCellIdx, int32_t* aRowIdx,
                                  int32_t* aColIdx) 
    { 
      uint32_t colCount = ColCount();
      *aRowIdx = aCellIdx / colCount;
      *aColIdx = aCellIdx % colCount;
    }

  



  virtual uint32_t ColExtentAt(uint32_t aRowIdx, uint32_t aColIdx) { return 1; }

  



  virtual uint32_t RowExtentAt(uint32_t aRowIdx, uint32_t aColIdx) { return 1; }

  


  virtual void ColDescription(uint32_t aColIdx, nsString& aDescription)
    { aDescription.Truncate(); }

  


  virtual void RowDescription(uint32_t aRowIdx, nsString& aDescription)
    { aDescription.Truncate(); }

  


  virtual bool IsColSelected(uint32_t aColIdx) { return false; }

  


  virtual bool IsRowSelected(uint32_t aRowIdx) { return false; }

  


  virtual bool IsCellSelected(uint32_t aRowIdx, uint32_t aColIdx) { return false; }

  


  virtual uint32_t SelectedCellCount() { return 0; }

  


  virtual uint32_t SelectedColCount() { return 0; }

  


  virtual uint32_t SelectedRowCount() { return 0; }

  


  virtual void SelectedCells(nsTArray<Accessible*>* aCells) = 0;

  


  virtual void SelectedCellIndices(nsTArray<uint32_t>* aCells) = 0;

  


  virtual void SelectedColIndices(nsTArray<uint32_t>* aCols) = 0;

  


  virtual void SelectedRowIndices(nsTArray<uint32_t>* aRows) = 0;

  


  virtual void SelectCol(uint32_t aColIdx) {}

  


  virtual void SelectRow(uint32_t aRowIdx) {}

  


  virtual void UnselectCol(uint32_t aColIdx) {}

  


  virtual void UnselectRow(uint32_t aRowIdx) {}

  


  virtual bool IsProbablyLayoutTable() { return false; }

  


  virtual Accessible* AsAccessible() = 0;
};

} 
} 

#endif
