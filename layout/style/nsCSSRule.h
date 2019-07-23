






































#ifndef nsCSSRule_h___
#define nsCSSRule_h___

#include "nsISupports.h"

class nsIStyleSheet;
class nsICSSStyleSheet;
class nsPresContext;
struct nsRuleData;
class nsICSSGroupRule;

class nsCSSRule {
public:
  nsCSSRule(void);
  nsCSSRule(const nsCSSRule& aCopy);
  virtual ~nsCSSRule(void);

  
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();
protected:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
public:

  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const;
  NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet);

  NS_IMETHOD SetParentRule(nsICSSGroupRule* aRule);

  
  
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

protected:
  nsICSSStyleSheet*   mSheet;                         
  nsICSSGroupRule*    mParentRule;
#ifdef DEBUG_REFS
  PRInt32 mInstance;
#endif
};

#endif 
