





































#include "nsCOMPtr.h"
#include "prtypes.h"
#include "nsAutoComplete.h"
#include "nsReadableUtils.h"
#include "nsIGenericFactory.h"





NS_IMPL_ISUPPORTS1(nsAutoCompleteItem, nsIAutoCompleteItem)

nsAutoCompleteItem::nsAutoCompleteItem()
{
}

nsAutoCompleteItem::~nsAutoCompleteItem()
{
}

NS_IMETHODIMP nsAutoCompleteItem::GetValue(nsAString& aValue)
{
    aValue.Assign(mValue);
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteItem::SetValue(const nsAString& aValue)
{
    mValue.Assign(aValue);
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteItem::GetComment(PRUnichar * *aComment)
{
    if (!aComment) return NS_ERROR_NULL_POINTER;
    *aComment = ToNewUnicode(mComment);
    return NS_OK;
}
NS_IMETHODIMP nsAutoCompleteItem::SetComment(const PRUnichar * aComment)
{
    mComment = aComment;
    return NS_OK;
}


NS_IMETHODIMP nsAutoCompleteItem::GetClassName(char * *aClassName)
{
    if (!aClassName) return NS_ERROR_NULL_POINTER;
    *aClassName = ToNewCString(mClassName);
    return NS_OK;
}
NS_IMETHODIMP nsAutoCompleteItem::SetClassName(const char * aClassName)
{
    mClassName.AssignWithConversion(aClassName);
    return NS_OK;
}


NS_IMETHODIMP nsAutoCompleteItem::GetParam(nsISupports * *aParam)
{
    if (!aParam) return NS_ERROR_NULL_POINTER;
    *aParam = mParam;
    NS_IF_ADDREF(*aParam);
    return NS_OK;
}
NS_IMETHODIMP nsAutoCompleteItem::SetParam(nsISupports * aParam)
{
    mParam = aParam;
    return NS_OK;
}





NS_IMPL_ISUPPORTS1(nsAutoCompleteResults, nsIAutoCompleteResults)

nsAutoCompleteResults::nsAutoCompleteResults() :
    mDefaultItemIndex(0)
{
    NS_NewISupportsArray(getter_AddRefs(mItems));
}

nsAutoCompleteResults::~nsAutoCompleteResults()
{
}

NS_IMETHODIMP nsAutoCompleteResults::GetSearchString(PRUnichar * *aSearchString)
{
    if (!aSearchString) return NS_ERROR_NULL_POINTER;
    *aSearchString = ToNewUnicode(mSearchString);  
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteResults::SetSearchString(const PRUnichar * aSearchString)
{
    mSearchString = aSearchString;
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteResults::GetParam(nsISupports * *aParam)
{
    if (!aParam) return NS_ERROR_NULL_POINTER;
    *aParam = mParam;
    NS_IF_ADDREF(*aParam);
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteResults::SetParam(nsISupports * aParam)
{
    mParam = aParam;
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteResults::GetItems(nsISupportsArray * *aItems)
{
    if (!aItems) return NS_ERROR_NULL_POINTER;
    *aItems = mItems;
    NS_IF_ADDREF(*aItems);   
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteResults::SetItems(nsISupportsArray * aItems)
{
    mItems = aItems;
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteResults::GetDefaultItemIndex(PRInt32 *aDefaultItemIndex)
{
    if (!aDefaultItemIndex) return NS_ERROR_NULL_POINTER;
    *aDefaultItemIndex = mDefaultItemIndex;  
    return NS_OK;
}

NS_IMETHODIMP nsAutoCompleteResults::SetDefaultItemIndex(PRInt32 aDefaultItemIndex)
{
    mDefaultItemIndex = aDefaultItemIndex;
    return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsAutoCompleteItem)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAutoCompleteResults)

static const nsModuleComponentInfo components[] = {
    {
        "AutoComplete Search Results",
        NS_AUTOCOMPLETERESULTS_CID,
        NS_AUTOCOMPLETERESULTS_CONTRACTID,
        nsAutoCompleteResultsConstructor
    },
    {
        "AutoComplete Search Item",
        NS_AUTOCOMPLETEITEM_CID,
        NS_AUTOCOMPLETEITEM_CONTRACTID,
        nsAutoCompleteItemConstructor
    }
};

NS_IMPL_NSGETMODULE(xpAutoComplete, components)
