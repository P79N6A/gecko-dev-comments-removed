




#include "nsContentSupportMap.h"
#include "nsXULElement.h"

void
nsContentSupportMap::Init()
{
    PL_DHashTableInit(&mMap, PL_DHashGetStubOps(), nullptr, sizeof(Entry));
}

void
nsContentSupportMap::Finish()
{
    if (mMap.ops)
        PL_DHashTableFinish(&mMap);
}

nsresult
nsContentSupportMap::Remove(nsIContent* aElement)
{
    if (!mMap.ops)
        return NS_ERROR_NOT_INITIALIZED;

    nsIContent* child = aElement;
    do {
        PL_DHashTableRemove(&mMap, child);
        child = child->GetNextNode(aElement);
    } while(child);

    return NS_OK;
}


