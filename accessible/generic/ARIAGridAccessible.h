




#ifndef MOZILLA_A11Y_ARIAGridAccessible_h_
#define MOZILLA_A11Y_ARIAGridAccessible_h_

#include "HyperTextAccessibleWrap.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"

namespace mozilla {
namespace a11y {




class ARIAGridAccessible : public AccessibleWrap,
                           public TableAccessible
{
public:
  ARIAGridAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual TableAccessible* AsTable() MOZ_OVERRIDE { return this; }

  
  virtual uint32_t ColCount() MOZ_OVERRIDE;
  virtual uint32_t RowCount() MOZ_OVERRIDE;
  virtual Accessible* CellAt(uint32_t aRowIndex, uint32_t aColumnIndex) MOZ_OVERRIDE;
  virtual bool IsColSelected(uint32_t aColIdx) MOZ_OVERRIDE;
  virtual bool IsRowSelected(uint32_t aRowIdx) MOZ_OVERRIDE;
  virtual bool IsCellSelected(uint32_t aRowIdx, uint32_t aColIdx) MOZ_OVERRIDE;
  virtual uint32_t SelectedCellCount() MOZ_OVERRIDE;
  virtual uint32_t SelectedColCount() MOZ_OVERRIDE;
  virtual uint32_t SelectedRowCount() MOZ_OVERRIDE;
  virtual void SelectedCells(nsTArray<Accessible*>* aCells) MOZ_OVERRIDE;
  virtual void SelectedCellIndices(nsTArray<uint32_t>* aCells) MOZ_OVERRIDE;
  virtual void SelectedColIndices(nsTArray<uint32_t>* aCols) MOZ_OVERRIDE;
  virtual void SelectedRowIndices(nsTArray<uint32_t>* aRows) MOZ_OVERRIDE;
  virtual void SelectCol(uint32_t aColIdx) MOZ_OVERRIDE;
  virtual void SelectRow(uint32_t aRowIdx) MOZ_OVERRIDE;
  virtual void UnselectCol(uint32_t aColIdx) MOZ_OVERRIDE;
  virtual void UnselectRow(uint32_t aRowIdx) MOZ_OVERRIDE;
  virtual Accessible* AsAccessible() MOZ_OVERRIDE { return this; }

protected:
  virtual ~ARIAGridAccessible() {}

  


  Accessible* GetRowAt(int32_t aRow);

  


  Accessible* GetCellInRowAt(Accessible* aRow, int32_t aColumn);

  







  nsresult SetARIASelected(Accessible* aAccessible, bool aIsSelected,
                           bool aNotify = true);
};





class ARIAGridCellAccessible : public HyperTextAccessibleWrap,
                               public TableCellAccessible
{
public:
  ARIAGridCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual TableCellAccessible* AsTableCell() MOZ_OVERRIDE { return this; }
  virtual void ApplyARIAState(uint64_t* aState) const MOZ_OVERRIDE;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;

protected:
  virtual ~ARIAGridCellAccessible() {}

  


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
