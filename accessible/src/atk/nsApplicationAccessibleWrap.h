








































#ifndef __NS_APP_ROOT_ACCESSIBLE_H__
#define __NS_APP_ROOT_ACCESSIBLE_H__

#include "nsApplicationAccessible.h"

class nsApplicationAccessibleWrap: public nsApplicationAccessible
{
public:
    static void Unload();
    static void PreCreate();

public:
    nsApplicationAccessibleWrap();
    virtual ~nsApplicationAccessibleWrap();

    
    virtual nsresult Init();

    
    NS_IMETHOD GetNativeInterface(void **aOutAccessible);

    
    virtual nsresult AddRootAccessible(nsIAccessible *aRootAccWrap);
    virtual nsresult RemoveRootAccessible(nsIAccessible *aRootAccWrap);
};

#endif   
