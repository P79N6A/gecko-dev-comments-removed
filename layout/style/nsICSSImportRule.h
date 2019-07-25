






































#ifndef nsICSSImportRule_h___
#define nsICSSImportRule_h___

#include "nsICSSRule.h"

class nsMediaList;
class nsString;


#define NS_ICSS_IMPORT_RULE_IID     \
{0x1d7a658b, 0x2f7b, 0x423d, {0xa3, 0xd9, 0xdd, 0x5b, 0x55, 0x3f, 0x69, 0xa9}}


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
