








































#ifndef __NS_APP_ROOT_ACCESSIBLE_H__
#define __NS_APP_ROOT_ACCESSIBLE_H__

#include "nsIMutableArray.h"
#include "nsIAccessibleDocument.h"
#include "nsAccessibilityService.h"
#include "nsAccessibleWrap.h"
#include "nsRootAccessibleWrap.h"

#define MAI_TYPE_APP_ROOT (MAI_TYPE_ATK_OBJECT)









class nsAppRootAccessible;
class nsAppRootAccessible: public nsAccessibleWrap
{
public:
    virtual ~nsAppRootAccessible();

    static nsAppRootAccessible *Create();
    static void Load();
    static void Unload();

public:
    nsAppRootAccessible();

    
    NS_IMETHOD Init();

    
    NS_IMETHOD GetName(nsAString & aName);
    NS_IMETHOD GetDescription(nsAString & aDescription);
    NS_IMETHOD GetRole(PRUint32 *aRole);
    NS_IMETHOD GetFinalRole(PRUint32 *aFinalRole);
    NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
    NS_IMETHOD GetParent(nsIAccessible * *aParent);
    NS_IMETHOD GetNextSibling(nsIAccessible * *aNextSibling);
    NS_IMETHOD GetPreviousSibling(nsIAccessible **aPreviousSibling);
    NS_IMETHOD GetChildAt(PRInt32 aChildNum, nsIAccessible **aChild);

    
    NS_IMETHOD GetNativeInterface(void **aOutAccessible);

    nsresult AddRootAccessible(nsIAccessible *aRootAccWrap);
    nsresult RemoveRootAccessible(nsIAccessible *aRootAccWrap);

protected:
    virtual void CacheChildren();

private:
    nsCOMPtr<nsIMutableArray> mChildren;
};

#endif   
