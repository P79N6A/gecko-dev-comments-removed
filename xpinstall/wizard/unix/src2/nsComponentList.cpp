






































#include "nsComponentList.h"
#include "nsComponent.h"

nsComponentList::nsComponentList() :
    mHeadItem(NULL),
    mTailItem(NULL),
    mNextItem(NULL),
    mLength(0)
{
}

nsComponentList::~nsComponentList()
{
    nsComponentItem *currItem = mHeadItem;
    nsComponentItem *nextItem = NULL;

    while (currItem)
    {
        nextItem = currItem->mNext;
        currItem->mComp->Release();
        delete currItem;
        currItem = nextItem;
    }

    mHeadItem = NULL;
    mTailItem = NULL;
    mLength = 0;
}

nsComponent *
nsComponentList::GetHead()
{
    if (mHeadItem)
    {
        mNextItem = mHeadItem->mNext;
        return mHeadItem->mComp;
    }

    return NULL;
}

nsComponent *
nsComponentList::GetNext()
{
    nsComponentItem *currItem = mNextItem;

    if (mNextItem)
    {
        mNextItem = mNextItem->mNext;
        return currItem->mComp;
    }

    return NULL;
}

nsComponent *
nsComponentList::GetTail()
{
    if (mTailItem)
        return mTailItem->mComp;

    return NULL;
}

int
nsComponentList::GetLength()
{
    return mLength;
}

int
nsComponentList::GetLengthVisible()
{
    int numVisible = 0;
    nsComponentItem *currItem;

    currItem = mHeadItem;
    if (!currItem) return 0;

    while (currItem)
    {
        if (!currItem->mComp->IsInvisible())
            numVisible++;
        currItem = currItem->mNext;
    }
        
    return numVisible;
}

int
nsComponentList::GetLengthSelected()
{
    int numSelected = 0;
    nsComponentItem *currItem;

    currItem = mHeadItem;
    if (!currItem) return 0;

    while (currItem)
    {
        if (currItem->mComp->IsSelected())
            numSelected++;
        currItem = currItem->mNext;
    }
        
    return numSelected;
}

int
nsComponentList::AddComponent(nsComponent *aComponent)
{
    if (!aComponent)
        return E_PARAM;

    aComponent->AddRef();
    nsComponentItem *newItem 
       = (nsComponentItem *) malloc(sizeof(nsComponentItem));
    newItem->mComp = aComponent;
    newItem->mNext = NULL;
    mLength++;
    
    if (mHeadItem)
    {
        
        mTailItem->mNext = newItem;
    }
    else
    {
        
        mHeadItem = newItem;
    }

    mTailItem = newItem;

    return OK;
}

int
nsComponentList::RemoveComponent(nsComponent *aComponent)
{
    nsComponentItem *currItem = mHeadItem;
    nsComponentItem *last = NULL;

    if (!aComponent)
        return E_PARAM;

    while (currItem)
    {
        if (aComponent == currItem->mComp)
        {
            
            if (last)
            {
                last->mNext = currItem->mNext;
            }
            else
            {
                mHeadItem = currItem->mNext;
            }

            if (mTailItem == currItem)
                mTailItem = NULL;
            if (mNextItem == currItem)
                mNextItem = mNextItem->mNext;

            aComponent->Release();
            mLength--;

            return OK;
        }
        else
        {
            
            last = currItem;
            currItem = currItem->mNext;
        }
    }

    return E_PARAM;
}

nsComponent *
nsComponentList::GetCompByIndex(int aIndex)
{
    nsComponentItem *currItem = mHeadItem;
    int i;

    
    if (!currItem || mLength == 0) return NULL;

    for (i=0; i<mLength; i++)
    { 
        if (aIndex == currItem->mComp->GetIndex())
        {
            return currItem->mComp;
        }

        currItem = currItem->mNext;
    }

    return NULL;
}

nsComponent *
nsComponentList::GetCompByArchive(char *aArchive)
{
    nsComponentItem *currItem = mHeadItem;
    int i;

    
    if (!currItem || mLength == 0 || !aArchive) return NULL;

    for (i=0; i<mLength; i++)
    { 
        if (0==strncmp(aArchive, currItem->mComp->GetArchive(), strlen(aArchive)))
        {
            return currItem->mComp;
        }

        currItem = currItem->mNext;
        if (!currItem) break;
    }

    return NULL;
}

nsComponent *
nsComponentList::GetCompByShortDesc(char *aShortDesc)
{
    nsComponentItem *currItem = mHeadItem;
    int i;

    
    if (!currItem || mLength == 0 || !aShortDesc) return NULL;

    for (i=0; i<mLength; i++)
    { 
        if (0==strncmp(aShortDesc, currItem->mComp->GetDescShort(), 
                       strlen(aShortDesc)))
        {
            return currItem->mComp;
        }

        currItem = currItem->mNext;
        if (!currItem) break;
    }

    return NULL;
}

nsComponent *
nsComponentList::GetFirstVisible()
{
    int i;
    nsComponentItem *currItem = mHeadItem;

    
    if (mLength == 0) return NULL;

    for (i=0; i<mLength; i++)
    { 
        if (!currItem->mComp->IsInvisible())
        {
            return currItem->mComp;
        }

        currItem = currItem->mNext;
        if (!currItem) break;
    }

    return NULL;
}
