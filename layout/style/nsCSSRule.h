






































#ifndef nsCSSRule_h___
#define nsCSSRule_h___

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsCSSStyleSheet.h"

class nsICSSGroupRule;

class nsCSSRule {
public:
  nsCSSRule(void)
    : mSheet(nsnull),
      mParentRule(nsnull)
  {
  }

  nsCSSRule(const nsCSSRule& aCopy)
    : mSheet(aCopy.mSheet),
      mParentRule(aCopy.mParentRule)
  {
  }

  already_AddRefed<nsIStyleSheet>
  GetStyleSheet() const
  {
    NS_IF_ADDREF(mSheet);
    return mSheet;
  }

  void
  SetStyleSheet(nsCSSStyleSheet* aSheet)
  {
    
    
    
    mSheet = aSheet;
  }

  void
  SetParentRule(nsICSSGroupRule* aRule)
  {
    
    
    
    mParentRule = aRule;
  }

protected:
  nsCSSStyleSheet*    mSheet;
  nsICSSGroupRule*    mParentRule;
};

#endif 
