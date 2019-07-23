





































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
    NS_IMETHOD GetState(PRUint32 *aState);
    void CacheChildren();
};

#endif  
