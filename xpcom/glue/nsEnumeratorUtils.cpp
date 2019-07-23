









































#include "nsEnumeratorUtils.h"

#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"

#include "nsCOMPtr.h"

class EmptyEnumeratorImpl : public nsISimpleEnumerator,
                            public nsIUTF8StringEnumerator,
                            public nsIStringEnumerator
{
public:
    
    NS_DECL_ISUPPORTS_INHERITED  

    
    NS_DECL_NSISIMPLEENUMERATOR
    NS_DECL_NSIUTF8STRINGENUMERATOR
    
    
    NS_IMETHOD GetNext(nsAString& aResult);

    static EmptyEnumeratorImpl* GetInstance() {
        return const_cast<EmptyEnumeratorImpl*>(&kInstance);
    }

private:
    static const EmptyEnumeratorImpl kInstance;
};


NS_IMETHODIMP_(nsrefcnt) EmptyEnumeratorImpl::AddRef(void)
{
    return 2;
}

NS_IMETHODIMP_(nsrefcnt) EmptyEnumeratorImpl::Release(void)
{
    return 1;
}

NS_IMPL_QUERY_INTERFACE1(EmptyEnumeratorImpl, nsISimpleEnumerator)


NS_IMETHODIMP EmptyEnumeratorImpl::HasMoreElements(PRBool* aResult)
{
    *aResult = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP EmptyEnumeratorImpl::HasMore(PRBool* aResult)
{
    *aResult = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP EmptyEnumeratorImpl::GetNext(nsISupports** aResult)
{
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP EmptyEnumeratorImpl::GetNext(nsACString& aResult)
{
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP EmptyEnumeratorImpl::GetNext(nsAString& aResult)
{
    return NS_ERROR_UNEXPECTED;
}

const EmptyEnumeratorImpl EmptyEnumeratorImpl::kInstance;

nsresult
NS_NewEmptyEnumerator(nsISimpleEnumerator** aResult)
{
    *aResult = EmptyEnumeratorImpl::GetInstance();
    return NS_OK;
}



class nsSingletonEnumerator : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD HasMoreElements(PRBool* aResult);
    NS_IMETHOD GetNext(nsISupports** aResult);

    nsSingletonEnumerator(nsISupports* aValue);

private:
    ~nsSingletonEnumerator();

protected:
    nsISupports* mValue;
    PRBool mConsumed;
};

nsSingletonEnumerator::nsSingletonEnumerator(nsISupports* aValue)
    : mValue(aValue)
{
    NS_IF_ADDREF(mValue);
    mConsumed = (mValue ? PR_FALSE : PR_TRUE);
}

nsSingletonEnumerator::~nsSingletonEnumerator()
{
    NS_IF_RELEASE(mValue);
}

NS_IMPL_ISUPPORTS1(nsSingletonEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsSingletonEnumerator::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != 0, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    *aResult = !mConsumed;
    return NS_OK;
}


NS_IMETHODIMP
nsSingletonEnumerator::GetNext(nsISupports** aResult)
{
    NS_PRECONDITION(aResult != 0, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (mConsumed)
        return NS_ERROR_UNEXPECTED;

    mConsumed = PR_TRUE;

    *aResult = mValue;
    NS_ADDREF(*aResult);
    return NS_OK;
}

nsresult
NS_NewSingletonEnumerator(nsISimpleEnumerator* *result,
                          nsISupports* singleton)
{
    nsSingletonEnumerator* enumer = new nsSingletonEnumerator(singleton);
    if (enumer == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    *result = enumer; 
    NS_ADDREF(*result);
    return NS_OK;
}



class nsUnionEnumerator : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD HasMoreElements(PRBool* aResult);
    NS_IMETHOD GetNext(nsISupports** aResult);

    nsUnionEnumerator(nsISimpleEnumerator* firstEnumerator,
                      nsISimpleEnumerator* secondEnumerator);

private:
    ~nsUnionEnumerator();

protected:
    nsCOMPtr<nsISimpleEnumerator> mFirstEnumerator, mSecondEnumerator;
    PRBool mConsumed;
    PRBool mAtSecond;
};

nsUnionEnumerator::nsUnionEnumerator(nsISimpleEnumerator* firstEnumerator,
                                     nsISimpleEnumerator* secondEnumerator)
    : mFirstEnumerator(firstEnumerator),
      mSecondEnumerator(secondEnumerator),
      mConsumed(PR_FALSE), mAtSecond(PR_FALSE)
{
}

nsUnionEnumerator::~nsUnionEnumerator()
{
}

NS_IMPL_ISUPPORTS1(nsUnionEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsUnionEnumerator::HasMoreElements(PRBool* aResult)
{
    NS_PRECONDITION(aResult != 0, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    if (mConsumed) {
        *aResult = PR_FALSE;
        return NS_OK;
    }

    if (! mAtSecond) {
        rv = mFirstEnumerator->HasMoreElements(aResult);
        if (NS_FAILED(rv)) return rv;

        if (*aResult)
            return NS_OK;

        mAtSecond = PR_TRUE;
    }

    rv = mSecondEnumerator->HasMoreElements(aResult);
    if (NS_FAILED(rv)) return rv;

    if (*aResult)
        return NS_OK;

    *aResult = PR_FALSE;
    mConsumed = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsUnionEnumerator::GetNext(nsISupports** aResult)
{
    NS_PRECONDITION(aResult != 0, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    if (mConsumed)
        return NS_ERROR_UNEXPECTED;

    if (! mAtSecond)
        return mFirstEnumerator->GetNext(aResult);

    return mSecondEnumerator->GetNext(aResult);
}

nsresult
NS_NewUnionEnumerator(nsISimpleEnumerator* *result,
                      nsISimpleEnumerator* firstEnumerator,
                      nsISimpleEnumerator* secondEnumerator)
{
    *result = nsnull;
    if (! firstEnumerator) {
        *result = secondEnumerator;
    } else if (! secondEnumerator) {
        *result = firstEnumerator;
    } else {
        nsUnionEnumerator* enumer = new nsUnionEnumerator(firstEnumerator, secondEnumerator);
        if (enumer == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        *result = enumer; 
    }
    NS_ADDREF(*result);
    return NS_OK;
}
