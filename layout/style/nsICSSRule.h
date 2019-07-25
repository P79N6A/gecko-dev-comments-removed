






































#ifndef nsICSSRule_h
#define nsICSSRule_h

#include "nsIStyleRule.h"
#include "nsIDOMCSSRule.h"

class nsCSSStyleSheet;
class nsICSSGroupRule;
class nsAString;
template<class T> struct already_AddRefed;


#define NS_ICSS_RULE_IID \
{ 0x1f560b20, 0xa829, 0x4b99, \
  { 0x87, 0xbd, 0x8c, 0x87, 0x95, 0x2b, 0x3b, 0xb6 } }



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

  


  virtual already_AddRefed<nsICSSRule> Clone() const = 0;

  
  
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
