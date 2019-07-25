




#ifndef MOZILLA_A11Y_ARIAGridAccessible_h_
#define MOZILLA_A11Y_ARIAGridAccessible_h_

#include "nsIAccessibleTable.h"

#include "HyperTextAccessibleWrap.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"
#include "xpcAccessibleTable.h"
#include "xpcAccessibleTableCell.h"

namespace mozilla {
namespace a11y {




class ARIAGridAccessible : public AccessibleWrap,
                           public xpcAccessibleTable,
                           public nsIAccessibleTable,
                           public TableAccessible
{
public:
  ARIAGridAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIACCESSIBLETABLE(xpcAccessibleTable::)

  
  virtual TableAccessible* AsTable() { return this; }

  
  virtual void Shutdown();

  
  virtual uint32_t ColCount();
  virtual uint32_t RowCount();
  virtual Accessible* CellAt(uint32_t aRowIndex, uint32_t aColumnIndex);
  virtual bool IsColSelected(uint32_t aColIdx);
  virtual bool IsRowSelected(uint32_t aRowIdx);
  virtual bool IsCellSelected(uint32_t aRowIdx, uint32_t aColIdx);
  virtual uint32_t SelectedCellCount();
  virtual uint32_t SelectedColCount();
  virtual uint32_t SelectedRowCount();
  virtual void SelectedCells(nsTArray<Accessible*>* aCells);
  virtual void SelectedCellIndices(nsTArray<uint32_t>* aCells);
  virtual void SelectedColIndices(nsTArray<uint32_t>* aCols);
  virtual void SelectedRowIndices(nsTArray<uint32_t>* aRows);
  virtual void SelectCol(uint32_t aColIdx);
  virtual void SelectRow(uint32_t aRowIdx);
  virtual void UnselectCol(uint32_t aColIdx);
  virtual void UnselectRow(uint32_t aRowIdx);
  virtual Accessible* AsAccessible() { return this; }

protected:

  


  bool IsValidRow(int32_t aRow);

  


  bool IsValidColumn(int32_t aColumn);

  


  Accessible* GetRowAt(int32_t aRow);

  


  Accessible* GetCellInRowAt(Accessible* aRow, int32_t aColumn);

  







  nsresult SetARIASelected(Accessible* aAccessible, bool aIsSelected,
                           bool aNotify = true);
};





class ARIAGridCellAccessible : public HyperTextAccessibleWrap,
                               public nsIAccessibleTableCell,
                               public TableCellAccessible,
                               public xpcAccessibleTableCell
{
public:
  ARIAGridCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIACCESSIBLETABLECELL(xpcAccessibleTableCell::)

  
  virtual TableCellAccessible* AsTableCell() { return this; }
  virtual void Shutdown();
  virtual void ApplyARIAState(uint64_t* aState) const;
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

protected:

  


  Accessible* Row() const
  {
    Accessible* row = Parent();
    return row && row->Role() == roles::ROW ? row : nullptr;
  }

  


  Accessible* TableFor(Accessible* aRow) const;

  


  int32_t RowIndexFor(Accessible* aRow) const;

  
  virtual TableAccessible* Table() const MOZ_OVERRIDE;
  virtual uint32_t ColIdx() const MOZ_OVERRIDE;
  virtual uint32_t RowIdx() const MOZ_OVERRIDE;
  virtual bool Selected() MOZ_OVERRIDE;
};

} 
} 

#endif
