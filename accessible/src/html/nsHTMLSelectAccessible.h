





































#ifndef __nsHTMLSelectAccessible_h__
#define __nsHTMLSelectAccessible_h__

#include "nsHTMLFormControlAccessible.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMNode.h"

class nsIMutableArray;



















class nsHTMLSelectListAccessible : public nsAccessibleWrap
{
public:
  
  nsHTMLSelectListAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsHTMLSelectListAccessible() {}

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsSelect();
  virtual bool SelectAll();
  virtual bool UnselectAll();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual nsAccessible* CurrentItem();

protected:

  
  virtual void CacheChildren();

  

  


  void CacheOptSiblings(nsIContent *aParentContent);
};




class nsHTMLSelectOptionAccessible : public nsHyperTextAccessibleWrap
{
public:
  enum { eAction_Select = 0 };  
  
  nsHTMLSelectOptionAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsHTMLSelectOptionAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD SetSelected(bool aSelect);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  virtual PRInt32 GetLevelInternal();
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);

  
  virtual PRUint8 ActionCount();

  
  virtual nsAccessible* ContainerWidget() const;

protected:
  
  virtual nsIFrame* GetBoundsFrame();

private:
  
  



 
  nsIContent* GetSelectState(PRUint64* aState);
};




class nsHTMLSelectOptGroupAccessible : public nsHTMLSelectOptionAccessible
{
public:

  nsHTMLSelectOptGroupAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsHTMLSelectOptGroupAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual PRUint32 NativeRole();
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

  nsHTMLComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsHTMLComboboxAccessible() {}

  
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual void Shutdown();

  
  virtual void Description(nsString& aDescription);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
  virtual void InvalidateChildren();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual nsAccessible* CurrentItem();

protected:
  
  virtual void CacheChildren();

  


  nsAccessible* SelectedOption() const;

private:
  nsRefPtr<nsHTMLComboboxListAccessible> mListAccessible;
};






class nsHTMLComboboxListAccessible : public nsHTMLSelectListAccessible
{
public:

  nsHTMLComboboxListAccessible(nsIAccessible *aParent, 
                               nsIContent *aContent, 
                               nsIWeakReference* aShell);
  virtual ~nsHTMLComboboxListAccessible() {}

  
  virtual nsIFrame* GetFrame() const;
  virtual bool IsPrimaryForNode() const;

  
  virtual PRUint64 NativeState();
  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);

  
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
};

#endif
