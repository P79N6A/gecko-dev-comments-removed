





































#ifndef _nsXFormsFormControlsAccessible_H_
#define _nsXFormsFormControlsAccessible_H_

#include "nsXFormsAccessible.h"





class nsXFormsLabelAccessible : public nsXFormsAccessible
{
public:
  nsXFormsLabelAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetDescription(nsAString& aDescription);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};





class nsXFormsOutputAccessible : public nsXFormsAccessible
{
public:
  nsXFormsOutputAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};





class nsXFormsTriggerAccessible : public nsXFormsAccessible
{
public:
  nsXFormsTriggerAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetValue(nsAString& aValue);

  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};





class nsXFormsInputAccessible : public nsXFormsEditableAccessible
{
public:
  nsXFormsInputAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};





class nsXFormsInputBooleanAccessible : public nsXFormsAccessible
{
public:
  nsXFormsInputBooleanAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};





class nsXFormsInputDateAccessible : public nsXFormsContainerAccessible
{
public:
  nsXFormsInputDateAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};





class nsXFormsSecretAccessible : public nsXFormsInputAccessible
{
public:
  nsXFormsSecretAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};






class nsXFormsRangeAccessible : public nsXFormsAccessible
{
public:
  nsXFormsRangeAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetMaximumValue(double *aMaximumValue);
  NS_IMETHOD GetMinimumValue(double *aMinimumValue);
  NS_IMETHOD GetMinimumIncrement(double *aMinimumIncrement);
  NS_IMETHOD GetCurrentValue(double *aCurrentValue);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};







class nsXFormsSelectAccessible : public nsXFormsContainerAccessible
{
public:
  nsXFormsSelectAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};






class nsXFormsChoicesAccessible : public nsXFormsAccessible
{
public:
  nsXFormsChoicesAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  void CacheChildren();
};







class nsXFormsSelectFullAccessible : public nsXFormsSelectableAccessible
{
public:
  nsXFormsSelectFullAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  void CacheChildren();
};








class nsXFormsItemCheckgroupAccessible : public nsXFormsSelectableItemAccessible
{
public:
  nsXFormsItemCheckgroupAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};








class nsXFormsItemRadiogroupAccessible : public nsXFormsSelectableItemAccessible
{
public:
  nsXFormsItemRadiogroupAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};







class nsXFormsSelectComboboxAccessible : public nsXFormsSelectableAccessible
{
public:
  nsXFormsSelectComboboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual PRBool GetAllowsAnonChildAccessibles();
};








class nsXFormsItemComboboxAccessible : public nsXFormsSelectableItemAccessible
{
public:
  nsXFormsItemComboboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

#endif

