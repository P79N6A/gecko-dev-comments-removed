






































#ifndef mozilla_css_Rule_h___
#define mozilla_css_Rule_h___

#include "nsICSSRule.h"

class nsIStyleSheet;
class nsCSSStyleSheet;
struct nsRuleData;
template<class T> struct already_AddRefed;

namespace mozilla {
namespace css {
class GroupRule;

#define DECL_STYLE_RULE_INHERIT_NO_DOMRULE  \
virtual void MapRuleInfoInto(nsRuleData* aRuleData);

#define DECL_STYLE_RULE_INHERIT  \
DECL_STYLE_RULE_INHERIT_NO_DOMRULE \
virtual nsIDOMCSSRule* GetDOMRuleWeak(nsresult* aResult);

class Rule : public nsICSSRule {
protected:
  Rule()
    : mSheet(nsnull),
      mParentRule(nsnull)
  {
  }

  Rule(const Rule& aCopy)
    : mSheet(aCopy.mSheet),
      mParentRule(aCopy.mParentRule)
  {
  }

public:

  virtual already_AddRefed<nsIStyleSheet> GetStyleSheet() const;
  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet);

  virtual void SetParentRule(GroupRule* aRule);

protected:
  nsCSSStyleSheet*  mSheet;
  GroupRule*        mParentRule;
};

} 
} 

#endif 
