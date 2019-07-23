




































 







#include "nsFindService.h"


nsFindService::nsFindService()
: mFindBackwards(PR_FALSE)
, mWrapFind(PR_FALSE)
, mEntireWord(PR_FALSE)
, mMatchCase(PR_FALSE)
{
}


nsFindService::~nsFindService()
{
}

NS_IMPL_ISUPPORTS1(nsFindService, nsIFindService)


NS_IMETHODIMP nsFindService::GetSearchString(nsAString & aSearchString)
{
    aSearchString = mSearchString;
    return NS_OK;
}

NS_IMETHODIMP nsFindService::SetSearchString(const nsAString & aSearchString)
{
    mSearchString = aSearchString;
    return NS_OK;
}


NS_IMETHODIMP nsFindService::GetReplaceString(nsAString & aReplaceString)
{
    aReplaceString = mReplaceString;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetReplaceString(const nsAString & aReplaceString)
{
    mReplaceString = aReplaceString;
    return NS_OK;
}


NS_IMETHODIMP nsFindService::GetFindBackwards(PRBool *aFindBackwards)
{
    NS_ENSURE_ARG_POINTER(aFindBackwards);
    *aFindBackwards = mFindBackwards;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetFindBackwards(PRBool aFindBackwards)
{
    mFindBackwards = aFindBackwards;
    return NS_OK;
}


NS_IMETHODIMP nsFindService::GetWrapFind(PRBool *aWrapFind)
{
    NS_ENSURE_ARG_POINTER(aWrapFind);
    *aWrapFind = mWrapFind;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetWrapFind(PRBool aWrapFind)
{
    mWrapFind = aWrapFind;
    return NS_OK;
}


NS_IMETHODIMP nsFindService::GetEntireWord(PRBool *aEntireWord)
{
    NS_ENSURE_ARG_POINTER(aEntireWord);
    *aEntireWord = mEntireWord;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetEntireWord(PRBool aEntireWord)
{
    mEntireWord = aEntireWord;
    return NS_OK;
}


NS_IMETHODIMP nsFindService::GetMatchCase(PRBool *aMatchCase)
{
    NS_ENSURE_ARG_POINTER(aMatchCase);
    *aMatchCase = mMatchCase;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetMatchCase(PRBool aMatchCase)
{
    mMatchCase = aMatchCase;
    return NS_OK;
}

