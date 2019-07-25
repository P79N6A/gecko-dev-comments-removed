






































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





class nsXULColumnsAccessible : public nsAccessibleWrap
{
public:
  nsXULColumnsAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
};





class nsXULColumnItemAccessible : public nsLeafAccessible
{
public:
  nsXULColumnItemAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
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
  nsXULListboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsXULListboxAccessible() {}

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLE_WITH_XPCACCESSIBLETABLE

  
  virtual PRUint32 ColCount();
  virtual PRUint32 RowCount();

  
  virtual void Shutdown();

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::TableAccessible* AsTable() { return this; }
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;

  virtual nsAccessible* ContainerWidget() const;

protected:
  bool IsMulticolumn();
};




class nsXULListitemAccessible : public nsXULMenuitemAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED
  
  nsXULListitemAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsXULListitemAccessible() {}

  
  NS_IMETHOD GetActionName(PRUint8 index, nsAString& aName);
  

  
  virtual void Description(nsString& aDesc);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual bool CanHaveAnonChildren();

  
  virtual nsAccessible* ContainerWidget() const;

protected:
  


  nsAccessible *GetListAccessible();

private:
  bool mIsCheckbox;
};




class nsXULListCellAccessible : public nsHyperTextAccessibleWrap,
                                public nsIAccessibleTableCell
{
public:
  nsXULListCellAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual mozilla::a11y::role NativeRole();
};

#endif
