






































#ifndef nsICSSImportRule_h
#define nsICSSImportRule_h

#include "nsICSSRule.h"

class nsMediaList;
class nsString;

#define NS_ICSS_IMPORT_RULE_IID \
{ 0x07bd9b80, 0x721e, 0x4566, \
  { 0xb7, 0x90, 0xed, 0x25, 0x10, 0xed, 0x99, 0xde } }


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
