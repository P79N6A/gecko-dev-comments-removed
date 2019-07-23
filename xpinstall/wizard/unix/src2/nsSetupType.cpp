






































#include "nsSetupType.h"

nsSetupType::nsSetupType() :
    mDescShort(NULL),
    mDescLong(NULL),
    mComponents(NULL),
    mNext(NULL)
{
}

nsSetupType::~nsSetupType()
{
    if (mDescShort)
        free (mDescShort);
    if (mDescLong)
        free (mDescLong);
    if (mComponents)   
        delete mComponents;
}

int
nsSetupType::SetDescShort(char *aDescShort)
{
    if (!aDescShort)
        return E_PARAM;
    
    mDescShort = aDescShort;

    return OK;
}

char *
nsSetupType::GetDescShort()
{
    if (mDescShort)
        return mDescShort;

    return NULL;
}

int
nsSetupType::SetDescLong(char *aDescLong)
{
    if (!aDescLong)
        return E_PARAM;
    
    mDescLong = aDescLong;

    return OK;
}

char *
nsSetupType::GetDescLong()
{
    if (mDescLong)
        return mDescLong;

    return NULL;
}

int
nsSetupType::SetComponent(nsComponent *aComponent)
{
    if (!aComponent)
        return E_PARAM;

    if (!mComponents)
        mComponents = new nsComponentList();

    if (!mComponents)
        return E_MEM;

    return mComponents->AddComponent(aComponent);
}

int
nsSetupType::UnsetComponent(nsComponent *aComponent)
{
    if (!aComponent)
        return E_PARAM;

    if (!mComponents)
        return OK;

    return mComponents->RemoveComponent(aComponent);
}

nsComponentList *
nsSetupType::GetComponents()
{
    if (mComponents)
        return mComponents;

    return NULL;
}

int
nsSetupType::SetNext(nsSetupType *aSetupType)
{
    if (!aSetupType)
        return E_PARAM;

    mNext = aSetupType;

    return OK;
}

nsSetupType *
nsSetupType::GetNext()
{
    if (mNext)
        return mNext;

    return NULL;
}
