






































#ifndef nsICSSImportRule_h___
#define nsICSSImportRule_h___

#include "nsICSSRule.h"

class nsMediaList;
class nsString;


#define NS_ICSS_IMPORT_RULE_IID     \
{0x99118ef3, 0x927d, 0x43f0, {0xa2, 0x10, 0x27, 0x48, 0x2d, 0x75, 0xde, 0x2e}}


class nsICSSImportRule : public nsICSSRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_IMPORT_RULE_IID)

  NS_IMETHOD SetURLSpec(const nsString& aURLSpec) = 0;
  NS_IMETHOD GetURLSpec(nsString& aURLSpec) const = 0;

  NS_IMETHOD SetMedia(const nsString& aMedia) = 0;
  NS_IMETHOD GetMedia(nsString& aMedia) const = 0;

  NS_IMETHOD SetSheet(nsCSSStyleSheet*) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSImportRule, NS_ICSS_IMPORT_RULE_IID)

nsresult
NS_NewCSSImportRule(nsICSSImportRule** aInstancePtrResult, 
                    const nsString& aURLSpec, nsMediaList* aMedia);

#endif 
