





































#ifndef nsAutoComplete_h___
#define nsAutoComplete_h___

#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsIAutoCompleteResults.h"





class nsAutoCompleteItem : public nsIAutoCompleteItem
{
public:
	nsAutoCompleteItem();
	virtual ~nsAutoCompleteItem();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETEITEM

private:
    nsString mValue;
    nsString mComment;
    nsCString mClassName;
    
    nsCOMPtr<nsISupports> mParam;
};




class nsAutoCompleteResults : public nsIAutoCompleteResults
{
public:
	nsAutoCompleteResults();
	virtual ~nsAutoCompleteResults();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETERESULTS
  
private:
    nsString mSearchString;    
    nsCOMPtr<nsISupportsArray> mItems;
    PRInt32 mDefaultItemIndex;

    nsCOMPtr<nsISupports> mParam;
};

#endif
