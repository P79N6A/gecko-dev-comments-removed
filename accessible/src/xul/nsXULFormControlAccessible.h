






































#ifndef _nsXULFormControlAccessible_H_
#define _nsXULFormControlAccessible_H_


#include "nsAccessibleWrap.h"
#include "nsFormControlAccessible.h"
#include "nsHyperTextAccessibleWrap.h"
#include "XULSelectControlAccessible.h"




typedef ProgressMeterAccessible<100> XULProgressMeterAccessible;







class nsXULButtonAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };
  nsXULButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual mozilla::a11y::role NativeRole();
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
  nsXULCheckboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();
};




class nsXULDropmarkerAccessible : public nsFormControlAccessible
{
public:
  enum { eAction_Click = 0 };
  nsXULDropmarkerAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

private:
  bool DropmarkerOpen(bool aToggleOpen);
};




class nsXULGroupboxAccessible : public nsAccessibleWrap
{
public:
  nsXULGroupboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual Relation RelationByType(PRUint32 aRelationType);
};




class nsXULRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsXULRadioButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);
  virtual PRUint64 NativeState();

  
  virtual nsAccessible* ContainerWidget() const;
};




class nsXULRadioGroupAccessible : public XULSelectControlAccessible
{
public:
  nsXULRadioGroupAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
};




class nsXULStatusBarAccessible : public nsAccessibleWrap
{
public:
  nsXULStatusBarAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
};




class nsXULToolbarButtonAccessible : public nsXULButtonAccessible
{
public:
  nsXULToolbarButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);

  
  static bool IsSeparator(nsAccessible *aAccessible);
};




class nsXULToolbarAccessible : public nsAccessibleWrap
{
public:
  nsXULToolbarAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual nsresult GetNameInternal(nsAString& aName);
};




class nsXULToolbarSeparatorAccessible : public nsLeafAccessible
{
public:
  nsXULToolbarSeparatorAccessible(nsIContent* aContent,
                                  nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
};




class nsXULTextFieldAccessible : public nsHyperTextAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsXULTextFieldAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  
  virtual void ApplyARIAState(PRUint64* aState);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual bool CanHaveAnonChildren();

  
  virtual PRUint8 ActionCount();

protected:
  
  virtual void CacheChildren();

  
  virtual already_AddRefed<nsFrameSelection> FrameSelection();

  
  already_AddRefed<nsIContent> GetInputField() const;
};


#endif

