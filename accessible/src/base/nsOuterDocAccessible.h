





































#ifndef _nsOuterDocAccessible_H_
#define _nsOuterDocAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsIAccessible.h"









class nsOuterDocAccessible : public nsAccessibleWrap
{
  
  
  NS_DECL_ISUPPORTS_INHERITED

public:
  nsOuterDocAccessible(nsIDOMNode* aNode, 
                       nsIWeakReference* aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetActionDescription(PRUint8 aIndex, nsAString& aDescription);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

protected:
  
  virtual void CacheChildren();
};

#endif  
