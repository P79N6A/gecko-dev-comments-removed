





#include "mozilla/Attributes.h"

#include "nsEnumeratorUtils.h"

#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"

#include "nsCOMPtr.h"

class EmptyEnumeratorImpl
  : public nsISimpleEnumerator
  , public nsIUTF8StringEnumerator
  , public nsIStringEnumerator
{
public:
  EmptyEnumeratorImpl() {}

  
  NS_DECL_ISUPPORTS_INHERITED  

  
  NS_DECL_NSISIMPLEENUMERATOR
  NS_DECL_NSIUTF8STRINGENUMERATOR
  
  
  NS_IMETHOD GetNext(nsAString& aResult);

  static EmptyEnumeratorImpl* GetInstance()
  {
    static const EmptyEnumeratorImpl kInstance;
    return const_cast<EmptyEnumeratorImpl*>(&kInstance);
  }
};


NS_IMETHODIMP_(MozExternalRefCountType)
EmptyEnumeratorImpl::AddRef(void)
{
  return 2;
}

NS_IMETHODIMP_(MozExternalRefCountType)
EmptyEnumeratorImpl::Release(void)
{
  return 1;
}

NS_IMPL_QUERY_INTERFACE(EmptyEnumeratorImpl, nsISimpleEnumerator,
                        nsIUTF8StringEnumerator, nsIStringEnumerator)


NS_IMETHODIMP
EmptyEnumeratorImpl::HasMoreElements(bool* aResult)
{
  *aResult = false;
  return NS_OK;
}

NS_IMETHODIMP
EmptyEnumeratorImpl::HasMore(bool* aResult)
{
  *aResult = false;
  return NS_OK;
}

NS_IMETHODIMP
EmptyEnumeratorImpl::GetNext(nsISupports** aResult)
{
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
EmptyEnumeratorImpl::GetNext(nsACString& aResult)
{
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
EmptyEnumeratorImpl::GetNext(nsAString& aResult)
{
  return NS_ERROR_UNEXPECTED;
}

nsresult
NS_NewEmptyEnumerator(nsISimpleEnumerator** aResult)
{
  *aResult = EmptyEnumeratorImpl::GetInstance();
  return NS_OK;
}



class nsSingletonEnumerator MOZ_FINAL : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD HasMoreElements(bool* aResult);
  NS_IMETHOD GetNext(nsISupports** aResult);

  explicit nsSingletonEnumerator(nsISupports* aValue);

private:
  ~nsSingletonEnumerator();

protected:
  nsISupports* mValue;
  bool mConsumed;
};

nsSingletonEnumerator::nsSingletonEnumerator(nsISupports* aValue)
  : mValue(aValue)
{
  NS_IF_ADDREF(mValue);
  mConsumed = (mValue ? false : true);
}

nsSingletonEnumerator::~nsSingletonEnumerator()
{
  NS_IF_RELEASE(mValue);
}

NS_IMPL_ISUPPORTS(nsSingletonEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsSingletonEnumerator::HasMoreElements(bool* aResult)
{
  NS_PRECONDITION(aResult != 0, "null ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  *aResult = !mConsumed;
  return NS_OK;
}


NS_IMETHODIMP
nsSingletonEnumerator::GetNext(nsISupports** aResult)
{
  NS_PRECONDITION(aResult != 0, "null ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  if (mConsumed) {
    return NS_ERROR_UNEXPECTED;
  }

  mConsumed = true;

  *aResult = mValue;
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
NS_NewSingletonEnumerator(nsISimpleEnumerator** aResult,
                          nsISupports* aSingleton)
{
  nsSingletonEnumerator* enumer = new nsSingletonEnumerator(aSingleton);
  if (!enumer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aResult = enumer;
  NS_ADDREF(*aResult);
  return NS_OK;
}



class nsUnionEnumerator MOZ_FINAL : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD HasMoreElements(bool* aResult);
  NS_IMETHOD GetNext(nsISupports** aResult);

  nsUnionEnumerator(nsISimpleEnumerator* aFirstEnumerator,
                    nsISimpleEnumerator* aSecondEnumerator);

private:
  ~nsUnionEnumerator();

protected:
  nsCOMPtr<nsISimpleEnumerator> mFirstEnumerator, mSecondEnumerator;
  bool mConsumed;
  bool mAtSecond;
};

nsUnionEnumerator::nsUnionEnumerator(nsISimpleEnumerator* aFirstEnumerator,
                                     nsISimpleEnumerator* aSecondEnumerator)
  : mFirstEnumerator(aFirstEnumerator)
  , mSecondEnumerator(aSecondEnumerator)
  , mConsumed(false)
  , mAtSecond(false)
{
}

nsUnionEnumerator::~nsUnionEnumerator()
{
}

NS_IMPL_ISUPPORTS(nsUnionEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsUnionEnumerator::HasMoreElements(bool* aResult)
{
  NS_PRECONDITION(aResult != 0, "null ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv;

  if (mConsumed) {
    *aResult = false;
    return NS_OK;
  }

  if (!mAtSecond) {
    rv = mFirstEnumerator->HasMoreElements(aResult);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (*aResult) {
      return NS_OK;
    }

    mAtSecond = true;
  }

  rv = mSecondEnumerator->HasMoreElements(aResult);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (*aResult) {
    return NS_OK;
  }

  *aResult = false;
  mConsumed = true;
  return NS_OK;
}

NS_IMETHODIMP
nsUnionEnumerator::GetNext(nsISupports** aResult)
{
  NS_PRECONDITION(aResult != 0, "null ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  if (mConsumed) {
    return NS_ERROR_UNEXPECTED;
  }

  if (!mAtSecond) {
    return mFirstEnumerator->GetNext(aResult);
  }

  return mSecondEnumerator->GetNext(aResult);
}

nsresult
NS_NewUnionEnumerator(nsISimpleEnumerator** aResult,
                      nsISimpleEnumerator* aFirstEnumerator,
                      nsISimpleEnumerator* aSecondEnumerator)
{
  *aResult = nullptr;
  if (!aFirstEnumerator) {
    *aResult = aSecondEnumerator;
  } else if (!aSecondEnumerator) {
    *aResult = aFirstEnumerator;
  } else {
    nsUnionEnumerator* enumer = new nsUnionEnumerator(aFirstEnumerator,
                                                      aSecondEnumerator);
    if (!enumer) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    *aResult = enumer;
  }
  NS_ADDREF(*aResult);
  return NS_OK;
}
