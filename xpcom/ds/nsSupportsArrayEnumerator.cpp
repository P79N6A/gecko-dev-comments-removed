




#include "nsSupportsArrayEnumerator.h"
#include "nsISupportsArray.h"

nsSupportsArrayEnumerator::nsSupportsArrayEnumerator(nsISupportsArray* array)
  : mArray(array), mCursor(0)
{
  NS_ASSERTION(array, "null array");
  NS_ADDREF(mArray);
}

nsSupportsArrayEnumerator::~nsSupportsArrayEnumerator()
{
  NS_RELEASE(mArray);
}

NS_IMPL_ISUPPORTS2(nsSupportsArrayEnumerator, nsIBidirectionalEnumerator, nsIEnumerator)

NS_IMETHODIMP
nsSupportsArrayEnumerator::First()
{
  mCursor = 0;
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  int32_t end = (int32_t)cnt;
  if (mCursor < end)
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::Next()
{
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  int32_t end = (int32_t)cnt;
  if (mCursor < end)   
    mCursor++;
  if (mCursor < end)
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::CurrentItem(nsISupports **aItem)
{
  NS_ASSERTION(aItem, "null out parameter");
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (mCursor >= 0 && mCursor < (int32_t)cnt) {
    *aItem = mArray->ElementAt(mCursor);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::IsDone()
{
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  
  
  return (mCursor >= 0 && mCursor < (int32_t)cnt)
    ? (nsresult)NS_ENUMERATOR_FALSE : NS_OK;
}



NS_IMETHODIMP
nsSupportsArrayEnumerator::Last()
{
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  mCursor = cnt - 1;
  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::Prev()
{
  if (mCursor >= 0)
    --mCursor;
  if (mCursor >= 0)
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}



nsresult
NS_NewISupportsArrayEnumerator(nsISupportsArray* array,
                               nsIBidirectionalEnumerator* *aInstancePtrResult)
{
  if (aInstancePtrResult == 0)
    return NS_ERROR_NULL_POINTER;
  nsSupportsArrayEnumerator* e = new nsSupportsArrayEnumerator(array);
  if (e == 0)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(e);
  *aInstancePtrResult = e;
  return NS_OK;
}



