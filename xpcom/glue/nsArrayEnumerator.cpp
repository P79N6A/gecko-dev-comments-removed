





































#include "nsArrayEnumerator.h"

#include "nsIArray.h"
#include "nsISimpleEnumerator.h"

#include "nsCOMArray.h"
#include "nsCOMPtr.h"

class nsSimpleArrayEnumerator : public nsISimpleEnumerator
{
public:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    
    nsSimpleArrayEnumerator(nsIArray* aValueArray) :
        mValueArray(aValueArray), mIndex(0) {
    }

private:
    ~nsSimpleArrayEnumerator() {}

protected:
    nsCOMPtr<nsIArray> mValueArray;
    PRUint32 mIndex;
};

NS_IMPL_ISUPPORTS1(nsSimpleArrayEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsSimpleArrayEnumerator::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != 0, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (!mValueArray) {
        *aResult = PR_FALSE;
        return NS_OK;
    }

    PRUint32 cnt;
    nsresult rv = mValueArray->GetLength(&cnt);
    if (NS_FAILED(rv)) return rv;
    *aResult = (mIndex < cnt);
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleArrayEnumerator::GetNext(nsISupports** aResult)
{
    NS_PRECONDITION(aResult != 0, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (!mValueArray) {
        *aResult = nsnull;
        return NS_OK;
    }

    PRUint32 cnt;
    nsresult rv = mValueArray->GetLength(&cnt);
    if (NS_FAILED(rv)) return rv;
    if (mIndex >= cnt)
        return NS_ERROR_UNEXPECTED;

    return mValueArray->QueryElementAt(mIndex++, NS_GET_IID(nsISupports), (void**)aResult);
}

nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator* *result,
                      nsIArray* array)
{
    nsSimpleArrayEnumerator* enumer = new nsSimpleArrayEnumerator(array);
    if (enumer == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*result = enumer);
    return NS_OK;
}







class nsCOMArrayEnumerator : public nsISimpleEnumerator
{
public:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    
    nsCOMArrayEnumerator() : mIndex(0) {
    }

    
    void* operator new (size_t size, const nsCOMArray_base& aArray) CPP_THROW_NEW;
    void operator delete(void* ptr) {
        ::operator delete(ptr);
    }

private:
    ~nsCOMArrayEnumerator(void);

protected:
    PRUint32 mIndex;            
    PRUint32 mArraySize;        
    
    
    nsISupports* mValueArray[1];
};

NS_IMPL_ISUPPORTS1(nsCOMArrayEnumerator, nsISimpleEnumerator)

nsCOMArrayEnumerator::~nsCOMArrayEnumerator()
{
    
    for (; mIndex < mArraySize; ++mIndex) {
        NS_IF_RELEASE(mValueArray[mIndex]);
    }
}

NS_IMETHODIMP
nsCOMArrayEnumerator::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != 0, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    *aResult = (mIndex < mArraySize);
    return NS_OK;
}

NS_IMETHODIMP
nsCOMArrayEnumerator::GetNext(nsISupports** aResult)
{
    NS_PRECONDITION(aResult != 0, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (mIndex >= mArraySize)
        return NS_ERROR_UNEXPECTED;

    
    
    
    *aResult = mValueArray[mIndex++];

    
    
    
    
    return NS_OK;
}

void*
nsCOMArrayEnumerator::operator new (size_t size, const nsCOMArray_base& aArray)
    CPP_THROW_NEW
{
    
    
    
    size += (aArray.Count() - 1) * sizeof(aArray[0]);

    
    nsCOMArrayEnumerator * result =
        static_cast<nsCOMArrayEnumerator*>(::operator new(size));
    NS_ENSURE_TRUE(result, nsnull);

    
    
    
    
    PRUint32 i;
    PRUint32 max = result->mArraySize = aArray.Count();
    for (i = 0; i<max; i++) {
        result->mValueArray[i] = aArray[i];
        NS_IF_ADDREF(result->mValueArray[i]);
    }

    return result;
}

nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator* *aResult,
                      const nsCOMArray_base& aArray)
{
    nsCOMArrayEnumerator *enumerator = new (aArray) nsCOMArrayEnumerator();
    if (!enumerator) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = enumerator);
    return NS_OK;
}
