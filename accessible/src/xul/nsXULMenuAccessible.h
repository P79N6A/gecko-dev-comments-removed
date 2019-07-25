




#ifndef _nsXULMenuAccessible_H_
#define _nsXULMenuAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "XULSelectControlAccessible.h"




class nsXULMenuitemAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsXULMenuitemAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRInt32 GetLevelInternal();

  virtual bool CanHaveAnonChildren();

  
  virtual PRUint8 ActionCount();
  virtual KeyBinding AccessKey() const;
  virtual KeyBinding KeyboardShortcut() const;

  
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual nsAccessible* ContainerWidget() const;
};




class nsXULMenuSeparatorAccessible : public nsXULMenuitemAccessible
{
public:
  nsXULMenuSeparatorAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();
};





class nsXULMenupopupAccessible : public XULSelectControlAccessible
{
public:
  nsXULMenupopupAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;

  virtual nsAccessible* ContainerWidget() const;
};




class nsXULMenubarAccessible : public nsAccessibleWrap
{
public:
  nsXULMenubarAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual nsAccessible* CurrentItem();
  virtual void SetCurrentItem(nsAccessible* aItem);
};

#endif
