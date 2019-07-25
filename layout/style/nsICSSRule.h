






































#ifndef nsICSSRule_h___
#define nsICSSRule_h___

#include "nsIStyleRule.h"
#include "nsIDOMCSSRule.h"

class nsCSSStyleSheet;
class nsICSSGroupRule;
class nsAString;


#define NS_ICSS_RULE_IID     \
{ 0x72250d73, 0xdbb2, 0x4409, \
 { 0x90, 0xfe, 0xe8, 0xe0, 0x28, 0x3a, 0x25, 0x10 } }


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

  NS_IMETHOD GetType(PRInt32& aType) const = 0;

  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const = 0;
  NS_IMETHOD SetStyleSheet(nsCSSStyleSheet* aSheet) = 0;
  NS_IMETHOD SetParentRule(nsICSSGroupRule* aRule) = 0;

  NS_IMETHOD Clone(nsICSSRule*& aClone) const = 0;

  
  
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
