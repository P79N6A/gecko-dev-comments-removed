






































#ifndef nsICSSRule_h___
#define nsICSSRule_h___

#include "nsIStyleRule.h"
#include "nsIDOMCSSRule.h"

class nsCSSStyleSheet;
class nsICSSGroupRule;
class nsAString;
template<class T> struct already_AddRefed;


#define NS_ICSS_RULE_IID     \
{ 0x471d733e, 0xc138, 0x4a50, \
 { 0x9e, 0x1a, 0xd1, 0x3c, 0xbb, 0x65, 0xb5, 0x26 } }



class nsICSSRule : public nsIStyleRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_RULE_IID)
  
  
  
  
  
  enum {
    UNKNOWN_RULE = 0,
    CHARSET_RULE,
    IMPORT_RULE,
    NAMESPACE_RULE,
    STYLE_RULE,
    MEDIA_RULE,
    FONT_FACE_RULE,
    PAGE_RULE,
    DOCUMENT_RULE
  };

  virtual PRInt32 GetType() const = 0;

  virtual already_AddRefed<nsIStyleSheet> GetStyleSheet() const = 0;
  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet) = 0;
  virtual void SetParentRule(nsICSSGroupRule* aRule) = 0;

  virtual nsresult Clone(nsICSSRule*& aClone) const = 0;

  
  
  nsresult GetDOMRule(nsIDOMCSSRule** aDOMRule)
  {
    nsresult rv;
    NS_IF_ADDREF(*aDOMRule = GetDOMRuleWeak(&rv));
    return rv;
  }
  virtual nsIDOMCSSRule* GetDOMRuleWeak(nsresult* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSRule, NS_ICSS_RULE_IID)



nsresult
NS_NewCSSCharsetRule(nsICSSRule** aInstancePtrResult,
                     const nsAString& aEncoding);

#endif 
