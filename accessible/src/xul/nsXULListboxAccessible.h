






































#ifndef __nsXULListboxAccessible_h__
#define __nsXULListboxAccessible_h__

#include "nsIAccessibleTable.h"

#include "nsCOMPtr.h"
#include "nsXULMenuAccessible.h"
#include "nsBaseWidgetAccessible.h"

class nsIWeakReference;





class nsXULColumnsAccessible : public nsAccessibleWrap
{
public:
  nsXULColumnsAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};





class nsXULColumnItemAccessible : public nsLeafAccessible
{
public:
  nsXULColumnItemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

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

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  PRBool IsMulticolumn();
};




class nsXULListitemAccessible : public nsXULMenuitemAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED
  
  nsXULListitemAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULListitemAccessible() {}

  
  NS_IMETHOD GetActionName(PRUint8 index, nsAString& aName);
  
  NS_IMETHOD GetDescription(nsAString& aDesc) { return nsAccessibleWrap::GetDescription(aDesc); }

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual PRBool GetAllowsAnonChildAccessibles();

protected:
  already_AddRefed<nsIAccessible> GetListAccessible();

private:
  PRBool mIsCheckbox;
};




class nsXULListCellAccessible : public nsHyperTextAccessibleWrap,
                                public nsIAccessibleTableCell
{
public:
  nsXULListCellAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

#endif
