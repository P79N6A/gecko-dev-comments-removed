




#include "nsCOMArray.h"
#include "nsCOMPtr.h"

static bool ReleaseObjects(void* aElement, void*);






nsCOMArray_base::nsCOMArray_base(const nsCOMArray_base& aOther)
{
    
    mArray.SizeTo(aOther.Count());
    AppendObjects(aOther);
}

nsCOMArray_base::~nsCOMArray_base()
{
  Clear();
}

int32_t
nsCOMArray_base::IndexOfObject(nsISupports* aObject) const {
    nsCOMPtr<nsISupports> supports = do_QueryInterface(aObject);
    NS_ENSURE_TRUE(supports, -1);

    int32_t i, count;
    int32_t retval = -1;
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

bool
nsCOMArray_base::InsertObjectAt(nsISupports* aObject, int32_t aIndex) {
    bool result = mArray.InsertElementAt(aObject, aIndex);
    if (result)
        NS_IF_ADDREF(aObject);
    return result;
}

bool
nsCOMArray_base::InsertObjectsAt(const nsCOMArray_base& aObjects, int32_t aIndex) {
    bool result = mArray.InsertElementsAt(aObjects.mArray, aIndex);
    if (result) {
        
        int32_t count = aObjects.Count();
        for (int32_t i = 0; i < count; ++i) {
            NS_IF_ADDREF(aObjects.ObjectAt(i));
        }
    }
    return result;
}

bool
nsCOMArray_base::ReplaceObjectAt(nsISupports* aObject, int32_t aIndex)
{
    
    nsISupports *oldObject =
        reinterpret_cast<nsISupports*>(mArray.SafeElementAt(aIndex));

    bool result = mArray.ReplaceElementAt(aObject, aIndex);

    
    
    if (result) {
        
        NS_IF_ADDREF(aObject);
        NS_IF_RELEASE(oldObject);
    }
    return result;
}

bool
nsCOMArray_base::RemoveObject(nsISupports *aObject)
{
    bool result = mArray.RemoveElement(aObject);
    if (result)
        NS_IF_RELEASE(aObject);
    return result;
}

bool
nsCOMArray_base::RemoveObjectAt(int32_t aIndex)
{
    if (uint32_t(aIndex) < uint32_t(Count())) {
        nsISupports* element = ObjectAt(aIndex);

        bool result = mArray.RemoveElementAt(aIndex);
        NS_IF_RELEASE(element);
        return result;
    }

    return false;
}

bool
nsCOMArray_base::RemoveObjectsAt(int32_t aIndex, int32_t aCount)
{
    if (uint32_t(aIndex) + uint32_t(aCount) <= uint32_t(Count())) {
        nsVoidArray elementsToDestroy(aCount);
        for (int32_t i = 0; i < aCount; ++i) {
            elementsToDestroy.InsertElementAt(mArray.FastElementAt(aIndex + i), i);
        }
        bool result = mArray.RemoveElementsAt(aIndex, aCount);
        for (int32_t i = 0; i < aCount; ++i) {
            nsISupports* element = static_cast<nsISupports*> (elementsToDestroy.FastElementAt(i));
            NS_IF_RELEASE(element);
        }
        return result;
    }

    return false;
}


bool
ReleaseObjects(void* aElement, void*)
{
    nsISupports* element = static_cast<nsISupports*>(aElement);
    NS_IF_RELEASE(element);
    return true;
}

void
nsCOMArray_base::Clear()
{
    nsAutoVoidArray objects;
    objects = mArray;
    mArray.Clear();
    objects.EnumerateForwards(ReleaseObjects, nullptr);
}

bool
nsCOMArray_base::SetCount(int32_t aNewCount)
{
    NS_ASSERTION(aNewCount >= 0,"SetCount(negative index)");
    if (aNewCount < 0)
      return false;

    int32_t count = Count(), i;
    nsAutoVoidArray objects;
    if (count > aNewCount) {
        objects.SetCount(count - aNewCount);
        for (i = aNewCount; i < count; ++i) {
            objects.ReplaceElementAt(ObjectAt(i), i - aNewCount);
        }
    }
    bool result = mArray.SetCount(aNewCount);
    objects.EnumerateForwards(ReleaseObjects, nullptr);
    return result;
}

