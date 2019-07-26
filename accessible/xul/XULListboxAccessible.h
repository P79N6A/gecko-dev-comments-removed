




#ifndef mozilla_a11y_XULListboxAccessible_h__
#define mozilla_a11y_XULListboxAccessible_h__

#include "BaseAccessibles.h"
#include "nsIAccessibleTable.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"
#include "xpcAccessibleTable.h"
#include "xpcAccessibleTableCell.h"
#include "XULMenuAccessible.h"
#include "XULSelectControlAccessible.h"

class nsIWeakReference;

namespace mozilla {
namespace a11y {





class XULColumAccessible : public AccessibleWrap
{
public:
  XULColumAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
};





class XULColumnItemAccessible : public LeafAccessible
{
public:
  XULColumnItemAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();

  
  virtual uint8_t ActionCount();

  enum { eAction_Click = 0 };
};




class XULListboxAccessible : public XULSelectControlAccessible,
                             public xpcAccessibleTable,
                             public nsIAccessibleTable,
                             public TableAccessible
{
public:
  XULListboxAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~XULListboxAccessible() {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIACCESSIBLETABLE(xpcAccessibleTable::)

  
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
  virtual void SelectRow(uint32_t aRowIdx);
  virtual void UnselectRow(uint32_t aRowIdx);
  virtual Accessible* AsAccessible() { return this; }

  
  virtual void Shutdown();
  virtual void Value(nsString& aValue);
  virtual TableAccessible* AsTable() { return this; }
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;

  virtual Accessible* ContainerWidget() const;

protected:
  bool IsMulticolumn();
};




class XULListitemAccessible : public XULMenuitemAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED

  XULListitemAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~XULListitemAccessible() {}

  
  NS_IMETHOD GetActionName(uint8_t index, nsAString& aName);
  

  
  virtual void Description(nsString& aDesc);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;
  virtual bool CanHaveAnonChildren();

  
  virtual Accessible* ContainerWidget() const;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;

  

  


  Accessible* GetListAccessible();

private:
  bool mIsCheckbox;
};




class XULListCellAccessible : public HyperTextAccessibleWrap,
                              public nsIAccessibleTableCell,
                              public TableCellAccessible,
                              public xpcAccessibleTableCell
{
public:
  XULListCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIACCESSIBLETABLECELL(xpcAccessibleTableCell::)

  
  virtual TableCellAccessible* AsTableCell() { return this; }
  virtual void Shutdown();
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;
  virtual a11y::role NativeRole();

  
  virtual TableAccessible* Table() const MOZ_OVERRIDE;
  virtual uint32_t ColIdx() const MOZ_OVERRIDE;
  virtual uint32_t RowIdx() const MOZ_OVERRIDE;
  virtual void ColHeaderCells(nsTArray<Accessible*>* aHeaderCells) MOZ_OVERRIDE;
  virtual bool Selected() MOZ_OVERRIDE;
};

} 
} 

#endif
