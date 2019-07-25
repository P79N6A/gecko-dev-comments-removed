



#ifndef __nsHTMLSelectAccessible_h__
#define __nsHTMLSelectAccessible_h__

#include "HTMLFormControlAccessible.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMNode.h"

class nsIMutableArray;



















class nsHTMLSelectListAccessible : public AccessibleWrap
{
public:
  
  nsHTMLSelectListAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsHTMLSelectListAccessible() {}

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsSelect();
  virtual bool SelectAll();
  virtual bool UnselectAll();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual Accessible* CurrentItem();
  virtual void SetCurrentItem(Accessible* aItem);

protected:

  
  virtual void CacheChildren();

  

  


  void CacheOptSiblings(nsIContent *aParentContent);
};




class nsHTMLSelectOptionAccessible : public HyperTextAccessibleWrap
{
public:
  enum { eAction_Select = 0 };  
  
  nsHTMLSelectOptionAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsHTMLSelectOptionAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD SetSelected(bool aSelect);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;

  virtual PRInt32 GetLevelInternal();
  virtual void GetBoundsRect(nsRect& aTotalBounds, nsIFrame** aBoundingFrame);

  
  virtual PRUint8 ActionCount();

  
  virtual Accessible* ContainerWidget() const;

private:
  
  

 
  Accessible* GetSelect() const
  {
    if (mParent && mParent->IsListControl()) {
      Accessible* combobox = mParent->Parent();
      return combobox && combobox->IsCombobox() ? combobox : mParent.get();
    }

    return nsnull;
  }

  


  Accessible* GetCombobox() const
  {
    if (mParent && mParent->IsListControl()) {
      Accessible* combobox = mParent->Parent();
      return combobox && combobox->IsCombobox() ? combobox : nsnull;
    }

    return nsnull;
  }
};




class nsHTMLSelectOptGroupAccessible : public nsHTMLSelectOptionAccessible
{
public:

  nsHTMLSelectOptGroupAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsHTMLSelectOptGroupAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeInteractiveState() const;

  
  virtual PRUint8 ActionCount();

protected:
  
  virtual void CacheChildren();
};





class nsHTMLComboboxListAccessible;




class nsHTMLComboboxAccessible : public AccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsHTMLComboboxAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsHTMLComboboxAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual void Shutdown();

  
  virtual void Description(nsString& aDescription);
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual void InvalidateChildren();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual Accessible* CurrentItem();
  virtual void SetCurrentItem(Accessible* aItem);

protected:
  
  virtual void CacheChildren();

  


  Accessible* SelectedOption() const;

private:
  nsRefPtr<nsHTMLComboboxListAccessible> mListAccessible;
};






class nsHTMLComboboxListAccessible : public nsHTMLSelectListAccessible
{
public:

  nsHTMLComboboxListAccessible(nsIAccessible* aParent, 
                               nsIContent* aContent, 
                               DocAccessible* aDoc);
  virtual ~nsHTMLComboboxListAccessible() {}

  
  virtual nsIFrame* GetFrame() const;
  virtual bool IsPrimaryForNode() const;

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);

  
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
};

#endif
