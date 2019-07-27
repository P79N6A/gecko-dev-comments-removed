





#include "mozilla/Attributes.h"

#include "nsArrayEnumerator.h"

#include "nsIArray.h"
#include "nsISimpleEnumerator.h"

#include "nsCOMArray.h"
#include "nsCOMPtr.h"

class nsSimpleArrayEnumerator MOZ_FINAL : public nsISimpleEnumerator
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSISIMPLEENUMERATOR

  
  explicit nsSimpleArrayEnumerator(nsIArray* aValueArray)
    : mValueArray(aValueArray)
    , mIndex(0)
  {
  }

private:
  ~nsSimpleArrayEnumerator() {}

protected:
  nsCOMPtr<nsIArray> mValueArray;
  uint32_t mIndex;
};

NS_IMPL_ISUPPORTS(nsSimpleArrayEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsSimpleArrayEnumerator::HasMoreElements(bool* aResult)
{
  NS_PRECONDITION(aResult != 0, "null ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  if (!mValueArray) {
    *aResult = false;
    return NS_OK;
  }

  uint32_t cnt;
  nsresult rv = mValueArray->GetLength(&cnt);
  if (NS_FAILED(rv)) {
    return rv;
  }
  *aResult = (mIndex < cnt);
  return NS_OK;
}

NS_IMETHODIMP
nsSimpleArrayEnumerator::GetNext(nsISupports** aResult)
{
  NS_PRECONDITION(aResult != 0, "null ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  if (!mValueArray) {
    *aResult = nullptr;
    return NS_OK;
  }

  uint32_t cnt;
  nsresult rv = mValueArray->GetLength(&cnt);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (mIndex >= cnt) {
    return NS_ERROR_UNEXPECTED;
  }

  return mValueArray->QueryElementAt(mIndex++, NS_GET_IID(nsISupports),
                                     (void**)aResult);
}

nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator** aResult, nsIArray* aArray)
{
  nsSimpleArrayEnumerator* enumer = new nsSimpleArrayEnumerator(aArray);
  if (!enumer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aResult = enumer);
  return NS_OK;
}







class nsCOMArrayEnumerator MOZ_FINAL : public nsISimpleEnumerator
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSISIMPLEENUMERATOR

  
  nsCOMArrayEnumerator() : mIndex(0) {}

  
  void* operator new(size_t aSize, const nsCOMArray_base& aArray) CPP_THROW_NEW;
  void operator delete(void* aPtr) { ::operator delete(aPtr); }

private:
  ~nsCOMArrayEnumerator(void);

protected:
  uint32_t mIndex;            
  uint32_t mArraySize;        

  
  nsISupports* mValueArray[1];
};

NS_IMPL_ISUPPORTS(nsCOMArrayEnumerator, nsISimpleEnumerator)

nsCOMArrayEnumerator::~nsCOMArrayEnumerator()
{
  
  for (; mIndex < mArraySize; ++mIndex) {
    NS_IF_RELEASE(mValueArray[mIndex]);
  }
}

NS_IMETHODIMP
nsCOMArrayEnumerator::HasMoreElements(bool* aResult)
{
  NS_PRECONDITION(aResult != 0, "null ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  *aResult = (mIndex < mArraySize);
  return NS_OK;
}

NS_IMETHODIMP
nsCOMArrayEnumerator::GetNext(nsISupports** aResult)
{
  NS_PRECONDITION(aResult != 0, "null ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  if (mIndex >= mArraySize) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  
  *aResult = mValueArray[mIndex++];

  
  
  

  return NS_OK;
}

void*
nsCOMArrayEnumerator::operator new(size_t aSize,
                                   const nsCOMArray_base& aArray) CPP_THROW_NEW
{
  
  
  
  aSize += (aArray.Count() - 1) * sizeof(aArray[0]);

  
  nsCOMArrayEnumerator* result =
    static_cast<nsCOMArrayEnumerator*>(::operator new(aSize));

  
  
  
  
  uint32_t i;
  uint32_t max = result->mArraySize = aArray.Count();
  for (i = 0; i < max; ++i) {
    result->mValueArray[i] = aArray[i];
    NS_IF_ADDREF(result->mValueArray[i]);
  }

  return result;
}

nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator** aResult,
                      const nsCOMArray_base& aArray)
{
  nsCOMArrayEnumerator* enumerator = new (aArray) nsCOMArrayEnumerator();
  if (!enumerator) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aResult = enumerator);
  return NS_OK;
}
