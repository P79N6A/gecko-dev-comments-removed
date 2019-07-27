




#include "nsContentSupportMap.h"
#include "nsXULElement.h"

void
nsContentSupportMap::Remove(nsIContent* aElement)
{
    nsIContent* child = aElement;
    do {
        PL_DHashTableRemove(&mMap, child);
        child = child->GetNextNode(aElement);
    } while(child);
}

