





































#include "nsContentSupportMap.h"
#include "nsXULElement.h"

void
nsContentSupportMap::Init()
{
    if (!PL_DHashTableInit(&mMap, PL_DHashGetStubOps(), nsnull,
                           sizeof(Entry), PL_DHASH_MIN_SIZE))
        mMap.ops = nsnull;
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
        PL_DHashTableOperate(&mMap, child, PL_DHASH_REMOVE);
        child = child->GetNextNode(aElement);
    } while(child);

    return NS_OK;
}


