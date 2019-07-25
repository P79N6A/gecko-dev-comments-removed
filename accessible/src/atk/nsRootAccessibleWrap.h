







































#ifndef __NS_ROOT_ACCESSIBLE_WRAP_H__
#define __NS_ROOT_ACCESSIBLE_WRAP_H__

#include "nsRootAccessible.h"

typedef nsRootAccessible nsRootAccessibleWrap;






class nsNativeRootAccessibleWrap: public nsRootAccessible
{
public:
    nsNativeRootAccessibleWrap(AtkObject *aAccessible);
    ~nsNativeRootAccessibleWrap();
};

#endif   
