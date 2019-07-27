




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

  
  virtual TableAccessible* AsTable() override { return this; }

  
  virtual uint32_t ColCount() override;
  virtual uint32_t RowCount() override;
  virtual Accessible* CellAt(uint32_t aRowIndex, uint32_t aColumnIndex) override;
  virtual bool IsColSelected(uint32_t aColIdx) override;
  virtual bool IsRowSelected(uint32_t aRowIdx) override;
  virtual bool IsCellSelected(uint32_t aRowIdx, uint32_t aColIdx) override;
  virtual uint32_t SelectedCellCount() override;
  virtual uint32_t SelectedColCount() override;
  virtual uint32_t SelectedRowCount() override;
  virtual void SelectedCells(nsTArray<Accessible*>* aCells) override;
  virtual void SelectedCellIndices(nsTArray<uint32_t>* aCells) override;
  virtual void SelectedColIndices(nsTArray<uint32_t>* aCols) override;
  virtual void SelectedRowIndices(nsTArray<uint32_t>* aRows) override;
  virtual void SelectCol(uint32_t aColIdx) override;
  virtual void SelectRow(uint32_t aRowIdx) override;
  virtual void UnselectCol(uint32_t aColIdx) override;
  virtual void UnselectRow(uint32_t aRowIdx) override;
  virtual Accessible* AsAccessible() override { return this; }

protected:
  virtual ~ARIAGridAccessible() {}

  


  Accessible* GetRowAt(int32_t aRow);

  


  Accessible* GetCellInRowAt(Accessible* aRow, int32_t aColumn);

  







  nsresult SetARIASelected(Accessible* aAccessible, bool aIsSelected,
                           bool aNotify = true);
};





class ARIARowAccessible : public AccessibleWrap
{
public:
  ARIARowAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual mozilla::a11y::GroupPos GroupPosition() override;

protected:
  virtual ~ARIARowAccessible() {}
};





class ARIAGridCellAccessible : public HyperTextAccessibleWrap,
                               public TableCellAccessible
{
public:
  ARIAGridCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual TableCellAccessible* AsTableCell() override { return this; }
  virtual void ApplyARIAState(uint64_t* aState) const override;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() override;
  virtual mozilla::a11y::GroupPos GroupPosition() override;

protected:
  virtual ~ARIAGridCellAccessible() {}

  


  Accessible* Row() const
  {
    Accessible* row = Parent();
    return row && row->IsTableRow() ? row : nullptr;
  }

  


  int32_t RowIndexFor(Accessible* aRow) const;

  
  virtual TableAccessible* Table() const override;
  virtual uint32_t ColIdx() const override;
  virtual uint32_t RowIdx() const override;
  virtual bool Selected() override;
};

} 
} 

#endif
