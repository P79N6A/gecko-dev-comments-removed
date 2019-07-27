




#ifndef mozilla_a11y_XULListboxAccessible_h__
#define mozilla_a11y_XULListboxAccessible_h__

#include "BaseAccessibles.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"
#include "xpcAccessibleTable.h"
#include "xpcAccessibleTableCell.h"
#include "XULMenuAccessible.h"
#include "XULSelectControlAccessible.h"

namespace mozilla {
namespace a11y {





class XULColumAccessible : public AccessibleWrap
{
public:
  XULColumAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
};





class XULColumnItemAccessible : public LeafAccessible
{
public:
  XULColumnItemAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  enum { eAction_Click = 0 };
};




class XULListboxAccessible : public XULSelectControlAccessible,
                             public TableAccessible
{
public:
  XULListboxAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
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
  virtual void SelectRow(uint32_t aRowIdx) override;
  virtual void UnselectRow(uint32_t aRowIdx) override;
  virtual Accessible* AsAccessible() override { return this; }

  
  virtual void Value(nsString& aValue) override;
  virtual TableAccessible* AsTable() override { return this; }
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;

  
  virtual bool IsWidget() const override;
  virtual bool IsActiveWidget() const override;
  virtual bool AreItemsOperable() const override;

  virtual Accessible* ContainerWidget() const override;

protected:
  virtual ~XULListboxAccessible() {}

  bool IsMulticolumn() { return ColCount() > 1; }
};




class XULListitemAccessible : public XULMenuitemAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED

  XULListitemAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Description(nsString& aDesc) override;
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual uint64_t NativeInteractiveState() const override;
  virtual bool CanHaveAnonChildren() override;

  
  virtual void ActionNameAt(uint8_t index, nsAString& aName) override;

  
  virtual Accessible* ContainerWidget() const override;

protected:
  virtual ~XULListitemAccessible();

  
  virtual ENameValueFlag NativeName(nsString& aName) override;

  

  


  Accessible* GetListAccessible() const;

private:
  bool mIsCheckbox;
};




class XULListCellAccessible : public HyperTextAccessibleWrap,
                              public TableCellAccessible
{
public:
  XULListCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual TableCellAccessible* AsTableCell() override { return this; }
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() override;
  virtual a11y::role NativeRole() override;

  
  virtual TableAccessible* Table() const override;
  virtual uint32_t ColIdx() const override;
  virtual uint32_t RowIdx() const override;
  virtual void ColHeaderCells(nsTArray<Accessible*>* aHeaderCells) override;
  virtual bool Selected() override;

protected:
  virtual ~XULListCellAccessible() {}
};

} 
} 

#endif
