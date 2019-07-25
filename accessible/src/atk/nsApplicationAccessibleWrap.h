







































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

    
    virtual bool Init();

    
    virtual bool AppendChild(nsAccessible* aChild);
    virtual bool RemoveChild(nsAccessible* aChild);

    
    NS_IMETHOD GetNativeInterface(void **aOutAccessible);
};

#endif   
