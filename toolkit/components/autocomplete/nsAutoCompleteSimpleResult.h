




































#ifndef __nsAutoCompleteSimpleResult__
#define __nsAutoCompleteSimpleResult__

#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteSimpleResult.h"

#include "nsString.h"
#include "prtypes.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

class nsAutoCompleteSimpleResult : public nsIAutoCompleteSimpleResult
{
public:
  nsAutoCompleteSimpleResult();
  inline void CheckInvariants() {
    NS_ASSERTION(mValues.Length() == mComments.Length(), "Arrays out of sync");
    NS_ASSERTION(mValues.Length() == mImages.Length(),   "Arrays out of sync");
    NS_ASSERTION(mValues.Length() == mStyles.Length(),   "Arrays out of sync");
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETERESULT
  NS_DECL_NSIAUTOCOMPLETESIMPLERESULT

private:
  ~nsAutoCompleteSimpleResult() {}

protected:

  
  
  
  nsTArray<nsString> mValues;
  nsTArray<nsString> mComments;
  nsTArray<nsString> mImages;
  nsTArray<nsString> mStyles;

  nsString mSearchString;
  nsString mErrorDescription;
  PRInt32 mDefaultIndex;
  PRUint32 mSearchResult;

  PRBool mIsURLResult;

  nsCOMPtr<nsIAutoCompleteSimpleResultListener> mListener;
};

#endif 
