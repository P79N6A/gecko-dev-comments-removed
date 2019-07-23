




































#ifndef __nsAutoCompleteResultBase__
#define __nsAutoCompleteResultBase__

#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteResultTypes.h"
#include "nsString.h"
#include "nsCOMArray.h"
#include "mdb.h"

class nsAutoCompleteMdbResult : public nsIAutoCompleteMdbResult2
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETERESULT

  nsAutoCompleteMdbResult();
  virtual ~nsAutoCompleteMdbResult();

  NS_DECL_NSIAUTOCOMPLETEBASERESULT
  NS_DECL_NSIAUTOCOMPLETEMDBRESULT
  NS_DECL_NSIAUTOCOMPLETEMDBRESULT2

protected:
  nsCOMArray<nsIMdbRow> mResults;

  nsAutoString mSearchString;
  nsAutoString mErrorDescription;
  PRInt32 mDefaultIndex;
  PRUint32 mSearchResult;
  
  nsIMdbEnv *mEnv;
  nsIMdbTable *mTable;
  
  mdb_scope mValueToken;
  PRInt16   mValueType;
  mdb_scope mCommentToken;
  PRInt16   mCommentType;

  PRPackedBool mReverseByteOrder;
};

#endif 
