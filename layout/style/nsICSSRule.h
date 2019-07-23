






































#ifndef nsICSSRule_h___
#define nsICSSRule_h___

#include "nsIStyleRule.h"

class nsICSSStyleSheet;
class nsICSSGroupRule;
class nsIDOMCSSRule;
class nsAString;


#define NS_ICSS_RULE_IID     \
{0xb9791e20, 0x1a04, 0x11d3, {0x80, 0x5a, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}


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

  
  
  NS_IMETHOD GetDOMRule(nsIDOMCSSRule** aDOMRule) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSRule, NS_ICSS_RULE_IID)



nsresult
NS_NewCSSCharsetRule(nsICSSRule** aInstancePtrResult,
                     const nsAString& aEncoding);

#endif 
