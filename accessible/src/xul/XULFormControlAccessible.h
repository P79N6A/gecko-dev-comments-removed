




#ifndef MOZILLA_A11Y_XULFormControlAccessible_H_
#define MOZILLA_A11Y_XULFormControlAccessible_H_


#include "nsAccessibleWrap.h"
#include "FormControlAccessible.h"
#include "nsHyperTextAccessibleWrap.h"
#include "XULSelectControlAccessible.h"

namespace mozilla {
namespace a11y {




typedef ProgressMeterAccessible<100> XULProgressMeterAccessible;







class XULButtonAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };
  XULButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
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





class XULCheckboxAccessible : public nsLeafAccessible
{
public:
  enum { eAction_Click = 0 };
  XULCheckboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();
};




class XULDropmarkerAccessible : public nsLeafAccessible
{
public:
  enum { eAction_Click = 0 };
  XULDropmarkerAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

private:
  bool DropmarkerOpen(bool aToggleOpen);
};




class XULGroupboxAccessible : public nsAccessibleWrap
{
public:
  XULGroupboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual Relation RelationByType(PRUint32 aRelationType);
};




class XULRadioButtonAccessible : public RadioButtonAccessible
{

public:
  XULRadioButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual PRUint64 NativeState();

  
  virtual nsAccessible* ContainerWidget() const;
};




class XULRadioGroupAccessible : public XULSelectControlAccessible
{
public:
  XULRadioGroupAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
};




class XULStatusBarAccessible : public nsAccessibleWrap
{
public:
  XULStatusBarAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
};




class XULToolbarButtonAccessible : public XULButtonAccessible
{
public:
  XULToolbarButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);

  
  static bool IsSeparator(nsAccessible *aAccessible);
};




class XULToolbarAccessible : public nsAccessibleWrap
{
public:
  XULToolbarAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual nsresult GetNameInternal(nsAString& aName);
};




class XULToolbarSeparatorAccessible : public nsLeafAccessible
{
public:
  XULToolbarSeparatorAccessible(nsIContent* aContent,
                                  nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
};




class XULTextFieldAccessible : public nsHyperTextAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  XULTextFieldAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual already_AddRefed<nsIEditor> GetEditor() const;

  
  virtual void Value(nsString& aValue);
  virtual void ApplyARIAState(PRUint64* aState) const;
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual bool CanHaveAnonChildren();

  
  virtual PRUint8 ActionCount();

protected:
  
  virtual void CacheChildren();

  
  virtual already_AddRefed<nsFrameSelection> FrameSelection();

  
  already_AddRefed<nsIContent> GetInputField() const;
};

} 
} 

#endif

