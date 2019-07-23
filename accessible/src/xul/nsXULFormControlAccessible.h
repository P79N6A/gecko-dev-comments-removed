






































#ifndef _nsXULFormControlAccessible_H_
#define _nsXULFormControlAccessible_H_


#include "nsAccessibleWrap.h"
#include "nsFormControlAccessible.h"
#include "nsXULSelectAccessible.h"
#include "nsHyperTextAccessibleWrap.h"

class nsXULButtonAccessible : public nsAccessibleWrap

{
public:
  enum { eAction_Click = 0 };
  nsXULButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  void CacheChildren();
};

class nsXULCheckboxAccessible : public nsFormControlAccessible
{
public:
  enum { eAction_Click = 0 };
  nsXULCheckboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsXULDropmarkerAccessible : public nsFormControlAccessible
{
public:
  enum { eAction_Click = 0 };
  nsXULDropmarkerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

private:
  PRBool DropmarkerOpen(PRBool aToggleOpen);
};

class nsXULGroupboxAccessible : public nsAccessibleWrap
{
public:
  nsXULGroupboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetName(nsAString& _retval);
  NS_IMETHOD GetAccessibleRelated(PRUint32 aRelationType, nsIAccessible **aRelated);
};

class nsXULProgressMeterAccessible : public nsFormControlAccessible
{
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEVALUE

public:
  nsXULProgressMeterAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aRole); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetValue(nsAString &aValue);
};

class nsXULRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsXULRadioButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

class nsXULRadioGroupAccessible : public nsXULSelectableAccessible
{
public:
  nsXULRadioGroupAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsXULStatusBarAccessible : public nsAccessibleWrap
{
public:
  nsXULStatusBarAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aRole);
};

class nsXULToolbarButtonAccessible : public nsXULButtonAccessible
{
public:
  nsXULToolbarButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  static PRBool IsSeparator(nsIAccessible *aAccessible);
};

class nsXULToolbarAccessible : public nsAccessibleWrap
{
public:
  nsXULToolbarAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsXULToolbarSeparatorAccessible : public nsLeafAccessible
{
public:
  nsXULToolbarSeparatorAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsXULTextFieldAccessible : public nsHyperTextAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsXULTextFieldAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

protected:
  already_AddRefed<nsIDOMNode> GetInputField();
};


#endif  

