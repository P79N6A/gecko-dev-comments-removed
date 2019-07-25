






































#ifndef mozilla_css_NameSpaceRule_h__
#define mozilla_css_NameSpaceRule_h__

#include "Rule.h"
#include "nsIDOMCSSRule.h"

class nsIAtom;


#define NS_CSS_NAMESPACE_RULE_IMPL_CID     \
{0xf0b0dbe1, 0x5031, 0x4a21, {0xb0, 0x6a, 0xdc, 0x14, 0x1e, 0xf2, 0xaf, 0x98}}


namespace mozilla {
namespace css {

class NS_FINAL_CLASS NameSpaceRule : public Rule,
                                     public nsIDOMCSSRule
{
public:
  NameSpaceRule();
private:
  
  NameSpaceRule(const NameSpaceRule& aCopy);
  ~NameSpaceRule();
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CSS_NAMESPACE_RULE_IMPL_CID)

  NS_DECL_ISUPPORTS

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<nsICSSRule> Clone() const;

  nsIAtom* GetPrefix() const { return mPrefix; }
  void SetPrefix(nsIAtom* aPrefix) { mPrefix = aPrefix; }

  void GetURLSpec(nsString& aURLSpec) const { aURLSpec = mURLSpec; }
  void SetURLSpec(const nsString& aURLSpec) { mURLSpec = aURLSpec; }

  
  NS_DECL_NSIDOMCSSRULE

private:
  nsCOMPtr<nsIAtom> mPrefix;
  nsString          mURLSpec;
};

} 
} 

NS_DEFINE_STATIC_IID_ACCESSOR(mozilla::css::NameSpaceRule, NS_CSS_NAMESPACE_RULE_IMPL_CID)

nsresult
NS_NewCSSNameSpaceRule(mozilla::css::NameSpaceRule** aInstancePtrResult,
                       nsIAtom* aPrefix, const nsString& aURLSpec);

#endif 
