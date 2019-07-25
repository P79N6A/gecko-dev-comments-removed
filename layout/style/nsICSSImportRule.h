






































#ifndef nsICSSImportRule_h___
#define nsICSSImportRule_h___

#include "nsICSSRule.h"
#include "nsString.h"

class nsIAtom;
class nsIURI;
class nsMediaList;


#define NS_ICSS_IMPORT_RULE_IID     \
{0xb2e65d15, 0x6673, 0x4548, {0xa6, 0x5a, 0xc4, 0x5c, 0xe8, 0x73, 0x04, 0xf2}}

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
