





































#ifndef __nsHTMLSelectAccessible_h__
#define __nsHTMLSelectAccessible_h__

#include "HTMLFormControlAccessible.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMNode.h"

class nsIMutableArray;



















class nsHTMLSelectListAccessible : public nsAccessibleWrap
{
public:
  
  nsHTMLSelectListAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsHTMLSelectListAccessible() {}

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsSelect();
  virtual bool SelectAll();
  virtual bool UnselectAll();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual nsAccessible* CurrentItem();
  virtual void SetCurrentItem(nsAccessible* aItem);

protected:

  
  virtual void CacheChildren();

  

  


  void CacheOptSiblings(nsIContent *aParentContent);
};




class nsHTMLSelectOptionAccessible : public nsHyperTextAccessibleWrap
{
public:
  enum { eAction_Select = 0 };  
  
  nsHTMLSelectOptionAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsHTMLSelectOptionAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD SetSelected(bool aSelect);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  virtual PRInt32 GetLevelInternal();
  virtual void GetBoundsRect(nsRect& aTotalBounds, nsIFrame** aBoundingFrame);

  
  virtual PRUint8 ActionCount();

  
  virtual nsAccessible* ContainerWidget() const;

private:
  
  

 
  nsAccessible* GetSelect() const
  {
    if (mParent && mParent->IsListControl()) {
      nsAccessible* combobox = mParent->Parent();
      return combobox && combobox->IsCombobox() ? combobox : mParent.get();
    }

    return nsnull;
  }

  


  nsAccessible* GetCombobox() const
  {
    if (mParent && mParent->IsListControl()) {
      nsAccessible* combobox = mParent->Parent();
      return combobox && combobox->IsCombobox() ? combobox : nsnull;
    }

    return nsnull;
  }
};




class nsHTMLSelectOptGroupAccessible : public nsHTMLSelectOptionAccessible
{
public:

  nsHTMLSelectOptGroupAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsHTMLSelectOptGroupAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

protected:
  
  virtual void CacheChildren();
};





class nsHTMLComboboxListAccessible;




class nsHTMLComboboxAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsHTMLComboboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
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
  virtual nsAccessible* CurrentItem();
  virtual void SetCurrentItem(nsAccessible* aItem);

protected:
  
  virtual void CacheChildren();

  


  nsAccessible* SelectedOption() const;

private:
  nsRefPtr<nsHTMLComboboxListAccessible> mListAccessible;
};






class nsHTMLComboboxListAccessible : public nsHTMLSelectListAccessible
{
public:

  nsHTMLComboboxListAccessible(nsIAccessible* aParent, 
                               nsIContent* aContent, 
                               nsDocAccessible* aDoc);
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
