






































#ifndef nsCSSRule_h___
#define nsCSSRule_h___

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsCSSStyleSheet.h"

class nsIStyleSheet;
class nsCSSStyleSheet;
struct nsRuleData;
template<class T> struct already_AddRefed;

namespace mozilla {
namespace css {
class GroupRule;
}
}

#define DECL_STYLE_RULE_INHERIT_NO_DOMRULE  \
virtual already_AddRefed<nsIStyleSheet> GetStyleSheet() const; \
virtual void SetStyleSheet(nsCSSStyleSheet* aSheet); \
virtual void SetParentRule(mozilla::css::GroupRule* aRule); \
virtual void MapRuleInfoInto(nsRuleData* aRuleData);

#define DECL_STYLE_RULE_INHERIT  \
DECL_STYLE_RULE_INHERIT_NO_DOMRULE \
virtual nsIDOMCSSRule* GetDOMRuleWeak(nsresult* aResult);

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
  SetParentRule(mozilla::css::GroupRule* aRule)
  {
    
    
    
    mParentRule = aRule;
  }

protected:
  nsCSSStyleSheet*         mSheet;
  mozilla::css::GroupRule* mParentRule;
};

#endif 
