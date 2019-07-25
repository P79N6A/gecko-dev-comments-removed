






































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

protected:

  
  virtual void CacheChildren();

  
  PRBool ContainsMenu();
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
  PRBool DropmarkerOpen(PRBool aToggleOpen);
};




class nsXULGroupboxAccessible : public nsAccessibleWrap
{
public:
  nsXULGroupboxAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetNameInternal(nsAString& aName);
};




class nsXULRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsXULRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);
  virtual PRUint64 NativeState();
};




class nsXULRadioGroupAccessible : public nsXULSelectableAccessible
{
public:
  nsXULRadioGroupAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
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

  
  static PRBool IsSeparator(nsAccessible *aAccessible);
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
  virtual PRBool GetAllowsAnonChildAccessibles();

  
  virtual PRUint8 ActionCount();

protected:
  
  virtual void CacheChildren();

  
  already_AddRefed<nsIContent> GetInputField() const;
};


#endif  

