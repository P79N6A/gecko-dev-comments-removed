






































#ifndef nsICSSImportRule_h___
#define nsICSSImportRule_h___

#include "nsICSSRule.h"
#include "nsString.h"

class nsIAtom;
class nsIURI;
class nsMediaList;


#define NS_ICSS_IMPORT_RULE_IID     \
{0x33824a60, 0x1a09, 0x11d3, {0x80, 0x5a, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

class nsICSSImportRule : public nsICSSRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_IMPORT_RULE_IID)

  NS_IMETHOD SetURLSpec(const nsString& aURLSpec) = 0;
  NS_IMETHOD GetURLSpec(nsString& aURLSpec) const = 0;

  NS_IMETHOD SetMedia(const nsString& aMedia) = 0;
  NS_IMETHOD GetMedia(nsString& aMedia) const = 0;

  NS_IMETHOD SetSheet(nsICSSStyleSheet*) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSImportRule, NS_ICSS_IMPORT_RULE_IID)

nsresult
NS_NewCSSImportRule(nsICSSImportRule** aInstancePtrResult, 
                    const nsString& aURLSpec, nsMediaList* aMedia);

#endif 
