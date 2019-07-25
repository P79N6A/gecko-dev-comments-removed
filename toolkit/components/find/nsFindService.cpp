




































 







#include "nsFindService.h"


nsFindService::nsFindService()
: mFindBackwards(false)
, mWrapFind(true)
, mEntireWord(false)
, mMatchCase(false)
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


NS_IMETHODIMP nsFindService::GetFindBackwards(bool *aFindBackwards)
{
    NS_ENSURE_ARG_POINTER(aFindBackwards);
    *aFindBackwards = mFindBackwards;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetFindBackwards(bool aFindBackwards)
{
    mFindBackwards = aFindBackwards;
    return NS_OK;
}


NS_IMETHODIMP nsFindService::GetWrapFind(bool *aWrapFind)
{
    NS_ENSURE_ARG_POINTER(aWrapFind);
    *aWrapFind = mWrapFind;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetWrapFind(bool aWrapFind)
{
    mWrapFind = aWrapFind;
    return NS_OK;
}


NS_IMETHODIMP nsFindService::GetEntireWord(bool *aEntireWord)
{
    NS_ENSURE_ARG_POINTER(aEntireWord);
    *aEntireWord = mEntireWord;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetEntireWord(bool aEntireWord)
{
    mEntireWord = aEntireWord;
    return NS_OK;
}


NS_IMETHODIMP nsFindService::GetMatchCase(bool *aMatchCase)
{
    NS_ENSURE_ARG_POINTER(aMatchCase);
    *aMatchCase = mMatchCase;
    return NS_OK;
}
NS_IMETHODIMP nsFindService::SetMatchCase(bool aMatchCase)
{
    mMatchCase = aMatchCase;
    return NS_OK;
}

