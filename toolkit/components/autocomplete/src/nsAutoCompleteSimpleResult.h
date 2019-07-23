




































#ifndef __nsAutoCompleteSimpleResult__
#define __nsAutoCompleteSimpleResult__

#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteSimpleResult.h"

#include "nsVoidArray.h"
#include "nsString.h"
#include "prtypes.h"

class nsAutoCompleteSimpleResult : public nsIAutoCompleteSimpleResult
{
public:
  nsAutoCompleteSimpleResult();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETERESULT
  NS_DECL_NSIAUTOCOMPLETESIMPLERESULT

private:
  ~nsAutoCompleteSimpleResult() {}

protected:

  
  
  
  nsStringArray mValues;
  nsStringArray mComments;

  nsString mSearchString;
  nsString mErrorDescription;
  PRInt32 mDefaultIndex;
  PRUint32 mSearchResult;
};

#endif 
