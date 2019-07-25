



#ifndef __nsXULListboxAccessible_h__
#define __nsXULListboxAccessible_h__

#include "nsCOMPtr.h"
#include "nsXULMenuAccessible.h"
#include "nsBaseWidgetAccessible.h"
#include "nsIAccessibleTable.h"
#include "TableAccessible.h"
#include "xpcAccessibleTable.h"
#include "XULSelectControlAccessible.h"

class nsIWeakReference;





class nsXULColumAccessible : public AccessibleWrap
{
public:
  nsXULColumAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
};





class nsXULColumnItemAccessible : public nsLeafAccessible
{
public:
  nsXULColumnItemAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  enum { eAction_Click = 0 };
};




class nsXULListboxAccessible : public XULSelectControlAccessible,
                               public xpcAccessibleTable,
                               public nsIAccessibleTable,
                               public mozilla::a11y::TableAccessible
{
public:
  nsXULListboxAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsXULListboxAccessible() {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLE_WITH_XPCACCESSIBLETABLE

  
  virtual PRUint32 ColCount();
  virtual PRUint32 RowCount();
  virtual Accessible* CellAt(PRUint32 aRowIndex, PRUint32 aColumnIndex);
  virtual void UnselectRow(PRUint32 aRowIdx);

  
  virtual void Shutdown();

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::TableAccessible* AsTable() { return this; }
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;

  virtual Accessible* ContainerWidget() const;

protected:
  bool IsMulticolumn();
};




class nsXULListitemAccessible : public nsXULMenuitemAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED
  
  nsXULListitemAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsXULListitemAccessible() {}

  
  NS_IMETHOD GetActionName(PRUint8 index, nsAString& aName);
  

  
  virtual void Description(nsString& aDesc);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;
  virtual bool CanHaveAnonChildren();

  
  virtual Accessible* ContainerWidget() const;

protected:
  


  Accessible* GetListAccessible();

private:
  bool mIsCheckbox;
};




class nsXULListCellAccessible : public HyperTextAccessibleWrap,
                                public nsIAccessibleTableCell
{
public:
  nsXULListCellAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual mozilla::a11y::role NativeRole();
};

#endif
