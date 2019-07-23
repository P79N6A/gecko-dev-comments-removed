






































#ifndef _nsXULTextAccessible_H_
#define _nsXULTextAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsHyperTextAccessibleWrap.h"

class nsXULTextAccessible : public nsHyperTextAccessibleWrap
{

public:
  nsXULTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsXULTooltipAccessible : public nsLeafAccessible
{

public:
  nsXULTooltipAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsXULLinkAccessible : public nsHyperTextAccessibleWrap
{

public:
  nsXULLinkAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& aValue);

  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_IMETHOD GetURI(PRInt32 aIndex, nsIURI **aURI);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  enum { eAction_Jump = 0 };

};

#endif  
