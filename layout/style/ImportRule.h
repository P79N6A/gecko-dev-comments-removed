






































#ifndef mozilla_css_ImportRule_h__
#define mozilla_css_ImportRule_h__

#include "Rule.h"
#include "nsIDOMCSSImportRule.h"
#include "nsCSSRules.h"

class nsMediaList;
class nsString;

namespace mozilla {
namespace css {

class NS_FINAL_CLASS ImportRule : public Rule,
                                  public nsIDOMCSSImportRule
{
public:
  ImportRule(nsMediaList* aMedia);
private:
  
  ImportRule(const ImportRule& aCopy);
  ~ImportRule();
public:
  NS_DECL_ISUPPORTS_INHERITED

  DECL_STYLE_RULE_INHERIT

#ifdef HAVE_CPP_AMBIGUITY_RESOLVING_USING
  using Rule::GetStyleSheet; 
#endif

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<nsICSSRule> Clone() const;

  void SetURLSpec(const nsString& aURLSpec) { mURLSpec = aURLSpec; }
  void GetURLSpec(nsString& aURLSpec) const { aURLSpec = mURLSpec; }

  nsresult SetMedia(const nsString& aMedia);
  void GetMedia(nsString& aMedia) const;

  void SetSheet(nsCSSStyleSheet*);

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSIMPORTRULE

private:
  nsString  mURLSpec;
  nsRefPtr<nsMediaList> mMedia;
  nsRefPtr<nsCSSStyleSheet> mChildSheet;
};

} 
} 

nsresult
NS_NewCSSImportRule(mozilla::css::ImportRule** aInstancePtrResult,
                    const nsString& aURLSpec, nsMediaList* aMedia);

#endif 
