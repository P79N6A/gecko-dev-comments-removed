








































#ifndef __NS_ROOT_ACCESSIBLE_WRAP_H__
#define __NS_ROOT_ACCESSIBLE_WRAP_H__

#include "nsRootAccessible.h"







class nsRootAccessibleWrap: public nsRootAccessible
{
public:
    nsRootAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference* aShell);
    virtual ~nsRootAccessibleWrap();

    NS_IMETHOD Init();
    NS_IMETHOD Shutdown();
    NS_IMETHOD GetParent(nsIAccessible **  aParent);
};


class nsNativeRootAccessibleWrap: public nsRootAccessible
{
public:
    nsNativeRootAccessibleWrap(AtkObject *aAccessible);
};

#endif   
