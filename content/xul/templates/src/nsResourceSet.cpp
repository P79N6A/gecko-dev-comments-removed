





































#include "nsResourceSet.h"

nsResourceSet::nsResourceSet(const nsResourceSet& aResourceSet)
    : mResources(nsnull),
      mCount(0),
      mCapacity(0)
{
    ConstIterator last = aResourceSet.Last();
    for (ConstIterator resource = aResourceSet.First(); resource != last; ++resource)
        Add(*resource);
}


nsResourceSet&
nsResourceSet::operator=(const nsResourceSet& aResourceSet)
{
    Clear();
    ConstIterator last = aResourceSet.Last();
    for (ConstIterator resource = aResourceSet.First(); resource != last; ++resource)
        Add(*resource);
    return *this;
}

nsResourceSet::~nsResourceSet()
{
    MOZ_COUNT_DTOR(nsResourceSet);
    Clear();
    delete[] mResources;
}

nsresult
nsResourceSet::Clear()
{
    while (--mCount >= 0) {
        NS_RELEASE(mResources[mCount]);
    }
    mCount = 0;
    return NS_OK;
}

nsresult
nsResourceSet::Add(nsIRDFResource* aResource)
{
    NS_PRECONDITION(aResource != nsnull, "null ptr");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;

    if (Contains(aResource))
        return NS_OK;

    if (mCount >= mCapacity) {
        PRInt32 capacity = mCapacity + 4;
        nsIRDFResource** resources = new nsIRDFResource*[capacity];
        if (! resources)
            return NS_ERROR_OUT_OF_MEMORY;

        for (PRInt32 i = mCount - 1; i >= 0; --i)
            resources[i] = mResources[i];

        delete[] mResources;

        mResources = resources;
        mCapacity = capacity;
    }

    mResources[mCount++] = aResource;
    NS_ADDREF(aResource);
    return NS_OK;
}

void
nsResourceSet::Remove(nsIRDFResource* aProperty)
{
    PRBool found = PR_FALSE;

    nsIRDFResource** res = mResources;
    nsIRDFResource** limit = mResources + mCount;
    while (res < limit) {
        if (found) {
            *(res - 1) = *res;
        }
        else if (*res == aProperty) {
            NS_RELEASE(*res);
            found = PR_TRUE;
        }
        ++res;
    }

    if (found)
        --mCount;
}

PRBool
nsResourceSet::Contains(nsIRDFResource* aResource) const
{
    for (PRInt32 i = mCount - 1; i >= 0; --i) {
        if (mResources[i] == aResource)
            return PR_TRUE;
    }

    return PR_FALSE;
}

