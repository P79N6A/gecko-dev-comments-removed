






































#ifndef mozilla_css_NameSpaceRule_h__
#define mozilla_css_NameSpaceRule_h__

#include "mozilla/css/Rule.h"
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
  NameSpaceRule(nsIAtom* aPrefix, const nsString& aURLSpec);
private:
  
  NameSpaceRule(const NameSpaceRule& aCopy);
  ~NameSpaceRule();
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CSS_NAMESPACE_RULE_IMPL_CID)

  NS_DECL_ISUPPORTS_INHERITED

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<Rule> Clone() const;

  nsIAtom* GetPrefix() const { return mPrefix; }

  void GetURLSpec(nsString& aURLSpec) const { aURLSpec = mURLSpec; }

  
  NS_DECL_NSIDOMCSSRULE

private:
  nsCOMPtr<nsIAtom> mPrefix;
  nsString          mURLSpec;
};

} 
} 

NS_DEFINE_STATIC_IID_ACCESSOR(mozilla::css::NameSpaceRule, NS_CSS_NAMESPACE_RULE_IMPL_CID)

#endif 
