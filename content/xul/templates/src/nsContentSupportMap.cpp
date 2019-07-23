





































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

    PL_DHashTableOperate(&mMap, aElement, PL_DHASH_REMOVE);

    PRUint32 count;

    
    
    
    nsXULElement *xulcontent = nsXULElement::FromContent(aElement);
    if (xulcontent) {
        count = xulcontent->PeekChildCount();
    }
    else {
        count = aElement->GetChildCount();
    }

    for (PRUint32 i = 0; i < count; ++i) {
        Remove(aElement->GetChildAt(i));
    }

    return NS_OK;
}


