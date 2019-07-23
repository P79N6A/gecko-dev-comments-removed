




































#ifndef __nsXULSelectAccessible_h__
#define __nsXULSelectAccessible_h__

#include "nsIAccessibleTable.h"

#include "nsCOMPtr.h"
#include "nsXULMenuAccessible.h"
#include "nsBaseWidgetAccessible.h"

class nsIWeakReference;





class nsXULColumnsAccessible : public nsAccessibleWrap
{
public:
  nsXULColumnsAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};





class nsXULColumnItemAccessible : public nsLeafAccessible
{
public:
  nsXULColumnItemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);

  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetNameInternal(nsAString& aName);

  enum { eAction_Click = 0 };
};























class nsXULListboxAccessible : public nsXULSelectableAccessible,
                               public nsIAccessibleTable
{
public:
  nsXULListboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULListboxAccessible() {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLE

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetValue(nsAString& aValue);

protected:
  PRBool IsTree();
};




class nsXULListitemAccessible : public nsXULMenuitemAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED
  
  nsXULListitemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULListitemAccessible() {}

  
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetActionName(PRUint8 index, nsAString& aName);
  
  NS_IMETHOD GetDescription(nsAString& aDesc) { return nsAccessibleWrap::GetDescription(aDesc); }
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

protected:
  already_AddRefed<nsIAccessible> GetListAccessible();

private:
  PRBool mIsCheckbox;
};




class nsXULListCellAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsXULListCellAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
};








class nsXULComboboxAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsXULComboboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULComboboxAccessible() {}

  
  NS_IMETHOD Init();

  
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 index, nsAString& aName);
};

#endif
