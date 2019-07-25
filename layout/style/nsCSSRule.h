






































#ifndef nsCSSRule_h___
#define nsCSSRule_h___

#include "nsISupports.h"

class nsIStyleSheet;
class nsCSSStyleSheet;
struct nsRuleData;
class nsICSSGroupRule;
template<class T> struct already_AddRefed;

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

  virtual already_AddRefed<nsIStyleSheet> GetStyleSheet() const;
  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet);

  virtual void SetParentRule(nsICSSGroupRule* aRule);

  
  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);

protected:
  nsCSSStyleSheet*    mSheet;
  nsICSSGroupRule*    mParentRule;
#ifdef DEBUG_REFS
  PRInt32 mInstance;
#endif
};

#endif 
