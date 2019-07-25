






































#ifndef mozilla_css_Rule_h___
#define mozilla_css_Rule_h___

#include "nsIStyleRule.h"
#include "nsIDOMCSSRule.h"

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

class Rule : public nsIStyleRule {
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

  virtual ~Rule() {}

public:
  
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();
protected:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
public:

  
  
  
  
  
  enum {
    UNKNOWN_RULE = 0,
    CHARSET_RULE,
    IMPORT_RULE,
    NAMESPACE_RULE,
    STYLE_RULE,
    MEDIA_RULE,
    FONT_FACE_RULE,
    PAGE_RULE,
#ifdef MOZ_CSS_ANIMATIONS
    KEYFRAME_RULE,
    KEYFRAMES_RULE,
#endif
    DOCUMENT_RULE
  };

  virtual PRInt32 GetType() const = 0;

  virtual nsIStyleSheet* GetStyleSheet() const;
  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet);
  virtual void SetParentRule(GroupRule* aRule);

  


  virtual already_AddRefed<Rule> Clone() const = 0;

  
  
  nsresult GetDOMRule(nsIDOMCSSRule** aDOMRule)
  {
    nsresult rv;
    NS_IF_ADDREF(*aDOMRule = GetDOMRuleWeak(&rv));
    return rv;
  }
  virtual nsIDOMCSSRule* GetDOMRuleWeak(nsresult* aResult) = 0;

protected:
  nsCSSStyleSheet*  mSheet;
  GroupRule*        mParentRule;
};

} 
} 

#endif
