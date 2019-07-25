







































#ifndef MOZILLA_A11Y_APPLICATION_ACCESSIBLE_WRAP_H__
#define MOZILLA_A11Y_APPLICATION_ACCESSIBLE_WRAP_H__

#include "ApplicationAccessible.h"

class ApplicationAccessibleWrap: public ApplicationAccessible
{
public:
    static void Unload();
    static void PreCreate();

public:
    ApplicationAccessibleWrap();
    virtual ~ApplicationAccessibleWrap();

    
    virtual bool Init();

    
    NS_IMETHOD GetName(nsAString &aName);

    virtual bool AppendChild(nsAccessible* aChild);
    virtual bool RemoveChild(nsAccessible* aChild);

    
    NS_IMETHOD GetNativeInterface(void **aOutAccessible);
};

#endif   
