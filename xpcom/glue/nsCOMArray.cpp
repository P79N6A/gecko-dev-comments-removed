





































#include "nsCOMArray.h"
#include "nsCOMPtr.h"

PR_STATIC_CALLBACK(PRBool) ReleaseObjects(void* aElement, void*);






nsCOMArray_base::nsCOMArray_base(const nsCOMArray_base& aOther)
{
    
    mArray.SizeTo(aOther.Count());
    AppendObjects(aOther);
}

nsCOMArray_base::~nsCOMArray_base()
{
    PRInt32 count = Count(), i;
    for (i = 0; i < count; ++i) {
        nsISupports* obj = ObjectAt(i);
        NS_IF_RELEASE(obj);
    }                        
}

PRInt32
nsCOMArray_base::IndexOfObject(nsISupports* aObject) const {
    nsCOMPtr<nsISupports> supports = do_QueryInterface(aObject);
    NS_ENSURE_TRUE(supports, -1);

    PRInt32 i, count;
    PRInt32 retval = -1;
    count = mArray.Count();
    for (i = 0; i < count; ++i) {
        nsCOMPtr<nsISupports> arrayItem =
            do_QueryInterface(reinterpret_cast<nsISupports*>(mArray.ElementAt(i)));
        if (arrayItem == supports) {
            retval = i;
            break;
        }
    }
    return retval;
}

PRBool
nsCOMArray_base::InsertObjectAt(nsISupports* aObject, PRInt32 aIndex) {
    PRBool result = mArray.InsertElementAt(aObject, aIndex);
    if (result)
        NS_IF_ADDREF(aObject);
    return result;
}

PRBool
nsCOMArray_base::InsertObjectsAt(const nsCOMArray_base& aObjects, PRInt32 aIndex) {
    PRBool result = mArray.InsertElementsAt(aObjects.mArray, aIndex);
    if (result) {
        
        PRInt32 count = aObjects.Count();
        for (PRInt32 i = 0; i < count; ++i) {
            NS_IF_ADDREF(aObjects.ObjectAt(i));
        }
    }
    return result;
}

PRBool
nsCOMArray_base::ReplaceObjectAt(nsISupports* aObject, PRInt32 aIndex)
{
    
    nsISupports *oldObject =
        reinterpret_cast<nsISupports*>(mArray.SafeElementAt(aIndex));

    PRBool result = mArray.ReplaceElementAt(aObject, aIndex);

    
    
    if (result) {
        
        NS_IF_ADDREF(aObject);
        NS_IF_RELEASE(oldObject);
    }
    return result;
}

PRBool
nsCOMArray_base::RemoveObject(nsISupports *aObject)
{
    PRBool result = mArray.RemoveElement(aObject);
    if (result)
        NS_IF_RELEASE(aObject);
    return result;
}

PRBool
nsCOMArray_base::RemoveObjectAt(PRInt32 aIndex)
{
    if (PRUint32(aIndex) < PRUint32(Count())) {
        nsISupports* element = ObjectAt(aIndex);
        NS_IF_RELEASE(element);

        return mArray.RemoveElementAt(aIndex);
    }

    return PR_FALSE;
}


PRBool
ReleaseObjects(void* aElement, void*)
{
    nsISupports* element = static_cast<nsISupports*>(aElement);
    NS_IF_RELEASE(element);
    return PR_TRUE;
}

void
nsCOMArray_base::Clear()
{
    mArray.EnumerateForwards(ReleaseObjects, nsnull);
    mArray.Clear();
}

