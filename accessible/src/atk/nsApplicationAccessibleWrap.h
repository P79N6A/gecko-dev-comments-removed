







































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

    
    virtual PRBool Init();

    
    virtual PRBool AppendChild(nsAccessible* aChild);
    virtual PRBool RemoveChild(nsAccessible* aChild);

    
    NS_IMETHOD GetNativeInterface(void **aOutAccessible);
};

#endif   
