





































#ifndef _nsOuterDocAccessible_H_
#define _nsOuterDocAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsIAccessible.h"

class nsIWeakReference;

class nsOuterDocAccessible : public nsAccessibleWrap
{
  NS_DECL_ISUPPORTS_INHERITED

  public:
    nsOuterDocAccessible(nsIDOMNode* aNode, 
                         nsIWeakReference* aShell);

    NS_IMETHOD GetName(nsAString& aName);
    NS_IMETHOD GetRole(PRUint32 *aRole);
    NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
    NS_IMETHOD GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                               nsIAccessible **aAccessible);
    void CacheChildren();
    nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

#endif  
