





#include "nsSupportsArrayEnumerator.h"
#include "nsISupportsArray.h"

nsSupportsArrayEnumerator::nsSupportsArrayEnumerator(nsISupportsArray* array)
  : mArray(array)
  , mCursor(0)
{
  NS_ASSERTION(array, "null array");
}

nsSupportsArrayEnumerator::~nsSupportsArrayEnumerator()
{
}

NS_IMPL_ISUPPORTS(nsSupportsArrayEnumerator, nsIBidirectionalEnumerator,
                  nsIEnumerator)

NS_IMETHODIMP
nsSupportsArrayEnumerator::First()
{
  mCursor = 0;
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) {
    return rv;
  }
  int32_t end = (int32_t)cnt;
  if (mCursor < end) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::Next()
{
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) {
    return rv;
  }
  int32_t end = (int32_t)cnt;
  if (mCursor < end) { 
    mCursor++;
  }
  if (mCursor < end) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::CurrentItem(nsISupports** aItem)
{
  NS_ASSERTION(aItem, "null out parameter");
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (mCursor >= 0 && mCursor < (int32_t)cnt) {
    return mArray->GetElementAt(mCursor, aItem);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::IsDone()
{
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  
  return (mCursor >= 0 && mCursor < (int32_t)cnt)
    ? (nsresult)NS_ENUMERATOR_FALSE : NS_OK;
}



NS_IMETHODIMP
nsSupportsArrayEnumerator::Last()
{
  uint32_t cnt;
  nsresult rv = mArray->Count(&cnt);
  if (NS_FAILED(rv)) {
    return rv;
  }
  mCursor = cnt - 1;
  return NS_OK;
}

NS_IMETHODIMP
nsSupportsArrayEnumerator::Prev()
{
  if (mCursor >= 0) {
    --mCursor;
  }
  if (mCursor >= 0) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

