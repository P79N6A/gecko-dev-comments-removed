






































#ifndef nsICSSRule_h___
#define nsICSSRule_h___

#include "nsIStyleRule.h"
#include "nsIDOMCSSRule.h"

class nsICSSStyleSheet;
class nsICSSGroupRule;
class nsAString;


#define NS_ICSS_RULE_IID     \
{0xe775eac0, 0xb022, 0x462c, {0xb1, 0xf9, 0x22, 0x1c, 0x01, 0xaa, 0x29, 0x88}}


class nsICSSRule : public nsIStyleRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_RULE_IID)
  enum {
    UNKNOWN_RULE = 0,
    STYLE_RULE = 1,
    IMPORT_RULE = 2,
    MEDIA_RULE = 3,
    FONT_FACE_RULE = 4,
    PAGE_RULE = 5,
    CHARSET_RULE = 6,
    NAMESPACE_RULE = 7,
    DOCUMENT_RULE = 8
  };

  NS_IMETHOD GetType(PRInt32& aType) const = 0;

  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const = 0;
  NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet) = 0;
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
