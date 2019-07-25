






































#include "nsCSSRule.h"
#include "nsCRT.h"
#include "nsCSSStyleSheet.h"

nsCSSRule::nsCSSRule(void)
  : mSheet(nsnull),
    mParentRule(nsnull)
{
}

nsCSSRule::nsCSSRule(const nsCSSRule& aCopy)
  : mSheet(aCopy.mSheet),
    mParentRule(aCopy.mParentRule)
{
}


nsCSSRule::~nsCSSRule(void)
{
}

NS_IMPL_ADDREF(nsCSSRule)
NS_IMPL_RELEASE(nsCSSRule)

NS_IMETHODIMP
nsCSSRule::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
  NS_IF_ADDREF(mSheet);
  aSheet = mSheet;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSRule::SetStyleSheet(nsCSSStyleSheet* aSheet)
{
  
  
  
  mSheet = aSheet;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSRule::SetParentRule(nsICSSGroupRule* aRule)
{
  
  
  
  mParentRule = aRule;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  
  NS_NOTREACHED("nsCSSRule::MapRuleInfoInto");
  return NS_OK;
}
