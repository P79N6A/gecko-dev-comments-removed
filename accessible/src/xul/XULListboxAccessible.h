




#ifndef mozilla_a11y_XULListboxAccessible_h__
#define mozilla_a11y_XULListboxAccessible_h__

#include "BaseAccessibles.h"
#include "nsIAccessibleTable.h"
#include "TableAccessible.h"
#include "xpcAccessibleTable.h"
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
  virtual PRUint64 NativeState();
};





class XULColumnItemAccessible : public LeafAccessible
{
public:
  XULColumnItemAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

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

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLE_WITH_XPCACCESSIBLETABLE

  
  virtual PRUint32 ColCount();
  virtual PRUint32 RowCount();
  virtual Accessible* CellAt(PRUint32 aRowIndex, PRUint32 aColumnIndex);
  virtual bool IsColSelected(PRUint32 aColIdx);
  virtual bool IsRowSelected(PRUint32 aRowIdx);
  virtual bool IsCellSelected(PRUint32 aRowIdx, PRUint32 aColIdx);
  virtual PRUint32 SelectedCellCount();
  virtual PRUint32 SelectedColCount();
  virtual PRUint32 SelectedRowCount();
  virtual void SelectedCellIndices(nsTArray<PRUint32>* aCells);
  virtual void SelectedColIndices(nsTArray<PRUint32>* aCols);
  virtual void SelectedRowIndices(nsTArray<PRUint32>* aRows);
  virtual void SelectRow(PRUint32 aRowIdx);
  virtual void UnselectRow(PRUint32 aRowIdx);

  
  virtual void Shutdown();

  
  virtual void Value(nsString& aValue);
  virtual TableAccessible* AsTable() { return this; }
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
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

  
  NS_IMETHOD GetActionName(PRUint8 index, nsAString& aName);
  

  
  virtual void Description(nsString& aDesc);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;
  virtual bool CanHaveAnonChildren();

  
  virtual Accessible* ContainerWidget() const;

protected:
  


  Accessible* GetListAccessible();

private:
  bool mIsCheckbox;
};




class XULListCellAccessible : public HyperTextAccessibleWrap,
                              public nsIAccessibleTableCell
{
public:
  XULListCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual a11y::role NativeRole();
};

} 
} 

#endif
