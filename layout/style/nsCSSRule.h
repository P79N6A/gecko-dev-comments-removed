






































#ifndef nsCSSRule_h___
#define nsCSSRule_h___

#include "nsICSSRule.h"

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

class nsCSSRule : public nsICSSRule {
protected:
  nsCSSRule()
    : mSheet(nsnull),
      mParentRule(nsnull)
  {
  }

  nsCSSRule(const nsCSSRule& aCopy)
    : mSheet(aCopy.mSheet),
      mParentRule(aCopy.mParentRule)
  {
  }

public:

  virtual already_AddRefed<nsIStyleSheet> GetStyleSheet() const;
  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet);

  virtual void SetParentRule(mozilla::css::GroupRule* aRule);

protected:
  nsCSSStyleSheet*         mSheet;
  mozilla::css::GroupRule* mParentRule;
};

#endif 
