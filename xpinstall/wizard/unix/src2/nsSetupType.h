






































#ifndef _NS_SETUPTYPE_H_
#define _NS_SETUPTYPE_H_

#include "XIDefines.h"
#include "XIErrors.h"

#include "nsComponent.h"

class nsComponentList;

class nsSetupType
{
public:
    nsSetupType();
    ~nsSetupType();




    int             SetDescShort(char *aDescShort);
    char *          GetDescShort();
    int             SetDescLong(char *aDescLong);
    char *          GetDescLong();
    int             SetComponent(nsComponent *aComponent);
    int             UnsetComponent(nsComponent *aComponent);
    nsComponentList *GetComponents();
    int             SetNext(nsSetupType *aSetupType);
    nsSetupType *   GetNext();
    
private:
    char            *mDescShort;
    char            *mDescLong;
    nsComponentList *mComponents;
    nsSetupType     *mNext;
};

#endif 
