






































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

 already_AddRefed<nsIStyleSheet>
nsCSSRule::GetStyleSheet() const
{
  NS_IF_ADDREF(mSheet);
  return mSheet;
}

 void
nsCSSRule::SetStyleSheet(nsCSSStyleSheet* aSheet)
{
  
  
  
  mSheet = aSheet;
}

 void
nsCSSRule::SetParentRule(nsICSSGroupRule* aRule)
{
  
  
  
  mParentRule = aRule;
}

 void
nsCSSRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  
  NS_NOTREACHED("nsCSSRule::MapRuleInfoInto");
}
