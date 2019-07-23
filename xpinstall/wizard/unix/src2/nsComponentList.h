






































#ifndef _NS_COMPONENTLIST_H_
#define _NS_COMPONENTLIST_H_

#include "XIErrors.h"

class nsComponent;

class nsComponentList
{
public:
    nsComponentList();
    ~nsComponentList();

    







    nsComponent *   GetHead();

    








    nsComponent *   GetNext(); 

    






    nsComponent *   GetTail();
    
    






    int             GetLength();

    






    int             GetLengthVisible();

    






    int             GetLengthSelected();

    







    int             AddComponent(nsComponent *aComponent);

    








    int             RemoveComponent(nsComponent *aComponent);

    








    nsComponent     *GetCompByIndex(int aIndex);

    








    nsComponent     *GetCompByArchive(char *aArchive);

    








    nsComponent     *GetCompByShortDesc(char *aShortDesc);

    







    nsComponent     *GetFirstVisible();

private:
    typedef struct _nsComponentItem
    {
       nsComponent             *mComp;
       struct _nsComponentItem *mNext;
    } nsComponentItem;

    nsComponentItem *mHeadItem; 
    nsComponentItem *mTailItem;
    nsComponentItem *mNextItem;
    int             mLength;
};

#endif 
