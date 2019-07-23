




































#include "nsAutoCompleteSimpleResult.h"

NS_IMPL_ISUPPORTS2(nsAutoCompleteSimpleResult,
                   nsIAutoCompleteResult,
                   nsIAutoCompleteSimpleResult)

nsAutoCompleteSimpleResult::nsAutoCompleteSimpleResult() :
  mDefaultIndex(-1),
  mSearchResult(RESULT_NOMATCH)
{
}


NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetSearchString(nsAString &aSearchString)
{
  aSearchString = mSearchString;
  return NS_OK;
}
NS_IMETHODIMP
nsAutoCompleteSimpleResult::SetSearchString(const nsAString &aSearchString)
{
  mSearchString.Assign(aSearchString);
  return NS_OK;
}


NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetSearchResult(PRUint16 *aSearchResult)
{
  *aSearchResult = mSearchResult;
  return NS_OK;
}
NS_IMETHODIMP
nsAutoCompleteSimpleResult::SetSearchResult(PRUint16 aSearchResult)
{
  mSearchResult = aSearchResult;
  return NS_OK;
}


NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetDefaultIndex(PRInt32 *aDefaultIndex)
{
  *aDefaultIndex = mDefaultIndex;
  return NS_OK;
}
NS_IMETHODIMP
nsAutoCompleteSimpleResult::SetDefaultIndex(PRInt32 aDefaultIndex)
{
  mDefaultIndex = aDefaultIndex;
  return NS_OK;
}


NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetErrorDescription(nsAString & aErrorDescription)
{
  aErrorDescription = mErrorDescription;
  return NS_OK;
}
NS_IMETHODIMP
nsAutoCompleteSimpleResult::SetErrorDescription(
                                             const nsAString &aErrorDescription)
{
  mErrorDescription.Assign(aErrorDescription);
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteSimpleResult::AppendMatch(const nsAString& aValue,
                                        const nsAString& aComment,
                                        const nsAString& aImage,
                                        const nsAString& aStyle)
{
  NS_ASSERTION(mValues.Count() == mComments.Count(), "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mImages.Count(),   "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mStyles.Count(),   "Arrays out of sync");

  if (! mValues.AppendString(aValue))
    return NS_ERROR_OUT_OF_MEMORY;
  if (! mComments.AppendString(aComment)) {
    mValues.RemoveStringAt(mValues.Count() - 1);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (! mImages.AppendString(aImage)) {
    mValues.RemoveStringAt(mValues.Count() - 1);
    mComments.RemoveStringAt(mComments.Count() - 1);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (! mStyles.AppendString(aStyle)) {
    mValues.RemoveStringAt(mValues.Count() - 1);
    mComments.RemoveStringAt(mComments.Count() - 1);
    mImages.RemoveStringAt(mImages.Count() - 1);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetMatchCount(PRUint32 *aMatchCount)
{
  NS_ASSERTION(mValues.Count() == mComments.Count(), "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mImages.Count(),   "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mStyles.Count(),   "Arrays out of sync");

  *aMatchCount = mValues.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetValueAt(PRInt32 aIndex, nsAString& _retval)
{
  NS_ENSURE_TRUE(aIndex >= 0 && aIndex < mValues.Count(),
                 NS_ERROR_ILLEGAL_VALUE);
  NS_ASSERTION(mValues.Count() == mComments.Count(), "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mImages.Count(),   "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mStyles.Count(),   "Arrays out of sync");
  mValues.StringAt(aIndex, _retval);
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetCommentAt(PRInt32 aIndex, nsAString& _retval)
{
  NS_ENSURE_TRUE(aIndex >= 0 && aIndex < mComments.Count(),
                 NS_ERROR_ILLEGAL_VALUE);
  NS_ASSERTION(mValues.Count() == mComments.Count(), "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mImages.Count(),   "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mStyles.Count(),   "Arrays out of sync");
  mComments.StringAt(aIndex, _retval);
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetImageAt(PRInt32 aIndex, nsAString& _retval)
{
  NS_ENSURE_TRUE(aIndex >= 0 && aIndex < mImages.Count(),
                 NS_ERROR_ILLEGAL_VALUE);
  NS_ASSERTION(mValues.Count() == mComments.Count(), "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mImages.Count(),   "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mStyles.Count(),   "Arrays out of sync");
  mImages.StringAt(aIndex, _retval);
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteSimpleResult::GetStyleAt(PRInt32 aIndex, nsAString& _retval)
{
  NS_ENSURE_TRUE(aIndex >= 0 && aIndex < mStyles.Count(),
                 NS_ERROR_ILLEGAL_VALUE);
  NS_ASSERTION(mValues.Count() == mComments.Count(), "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mImages.Count(),   "Arrays out of sync");
  NS_ASSERTION(mValues.Count() == mStyles.Count(),   "Arrays out of sync");
  mStyles.StringAt(aIndex, _retval);
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteSimpleResult::SetListener(nsIAutoCompleteSimpleResultListener* aListener)
{
  mListener = aListener;
  return NS_OK;
}

NS_IMETHODIMP
nsAutoCompleteSimpleResult::RemoveValueAt(PRInt32 aRowIndex,
                                          PRBool aRemoveFromDb)
{
  NS_ENSURE_TRUE(aRowIndex >= 0 && aRowIndex < mValues.Count(),
                 NS_ERROR_ILLEGAL_VALUE);

  nsAutoString removedValue(*mValues.StringAt(aRowIndex));
  mValues.RemoveStringAt(aRowIndex);
  mComments.RemoveStringAt(aRowIndex);
  mImages.RemoveStringAt(aRowIndex);
  mStyles.RemoveStringAt(aRowIndex);

  if (mListener)
    mListener->OnValueRemoved(this, removedValue, aRemoveFromDb);

  return NS_OK;
}
