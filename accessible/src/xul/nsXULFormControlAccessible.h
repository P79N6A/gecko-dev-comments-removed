






































#ifndef _nsXULFormControlAccessible_H_
#define _nsXULFormControlAccessible_H_


#include "nsAccessibleWrap.h"
#include "nsFormControlAccessible.h"
#include "nsXULMenuAccessible.h"
#include "nsHyperTextAccessibleWrap.h"




typedef ProgressMeterAccessible<100> XULProgressMeterAccessible;







class nsXULButtonAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };
  nsXULButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
  virtual nsAccessible* ContainerWidget() const;

protected:

  
  virtual void CacheChildren();

  
  bool ContainsMenu();
};





class nsXULCheckboxAccessible : public nsFormControlAccessible
{
public:
  enum { eAction_Click = 0 };
  nsXULCheckboxAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();
};




class nsXULDropmarkerAccessible : public nsFormControlAccessible
{
public:
  enum { eAction_Click = 0 };
  nsXULDropmarkerAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

private:
  bool DropmarkerOpen(bool aToggleOpen);
};




class nsXULGroupboxAccessible : public nsAccessibleWrap
{
public:
  nsXULGroupboxAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual Relation RelationByType(PRUint32 aRelationType);
};




class nsXULRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsXULRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);
  virtual PRUint64 NativeState();

  
  virtual nsAccessible* ContainerWidget() const;
};




class nsXULRadioGroupAccessible : public nsXULSelectableAccessible
{
public:
  nsXULRadioGroupAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
};




class nsXULStatusBarAccessible : public nsAccessibleWrap
{
public:
  nsXULStatusBarAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
};




class nsXULToolbarButtonAccessible : public nsXULButtonAccessible
{
public:
  nsXULToolbarButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);

  
  static bool IsSeparator(nsAccessible *aAccessible);
};




class nsXULToolbarAccessible : public nsAccessibleWrap
{
public:
  nsXULToolbarAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetNameInternal(nsAString& aName);
};




class nsXULToolbarSeparatorAccessible : public nsLeafAccessible
{
public:
  nsXULToolbarSeparatorAccessible(nsIContent* aContent,
                                  nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
};




class nsXULTextFieldAccessible : public nsHyperTextAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsXULTextFieldAccessible(nsIContent* aContent, nsIWeakReference *aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  
  virtual void ApplyARIAState(PRUint64* aState);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
  virtual bool GetAllowsAnonChildAccessibles();

  
  virtual PRUint8 ActionCount();

protected:
  
  virtual void CacheChildren();

  
  virtual already_AddRefed<nsFrameSelection> FrameSelection();

  
  already_AddRefed<nsIContent> GetInputField() const;
};


#endif  

