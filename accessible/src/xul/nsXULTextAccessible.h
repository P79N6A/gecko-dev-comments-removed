






































#ifndef _nsXULTextAccessible_H_
#define _nsXULTextAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsTextAccessibleWrap.h"
#include "nsHyperTextAccessible.h"

class nsIWeakReference;

class nsXULTextAccessible : public nsHyperTextAccessible
{

public:
  nsXULTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *aRole) { *aRole = nsIAccessibleRole::ROLE_LABEL; return NS_OK; }
};

class nsXULTooltipAccessible : public nsLeafAccessible
{

public:
  nsXULTooltipAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
};

class nsXULLinkAccessible : public nsLinkableAccessible
{

public:
  nsXULLinkAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetValue(nsAString& _retval);

protected:
  void CacheActionContent();
};

#endif  
