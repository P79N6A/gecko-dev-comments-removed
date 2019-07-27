




#include "nsContentSupportMap.h"
#include "nsXULElement.h"

nsresult
nsContentSupportMap::Remove(nsIContent* aElement)
{
    if (!mMap.IsInitialized())
        return NS_ERROR_NOT_INITIALIZED;

    nsIContent* child = aElement;
    do {
        PL_DHashTableRemove(&mMap, child);
        child = child->GetNextNode(aElement);
    } while(child);

    return NS_OK;
}


