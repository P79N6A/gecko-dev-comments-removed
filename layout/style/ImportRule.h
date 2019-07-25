






































#ifndef mozilla_css_ImportRule_h__
#define mozilla_css_ImportRule_h__

#include "nsICSSRule.h"
#include "nsCSSRule.h"
#include "nsIDOMCSSImportRule.h"
#include "nsCSSRules.h"

class nsMediaList;
class nsString;

namespace mozilla {
namespace css {

class NS_FINAL_CLASS ImportRule : public nsCSSRule,
                                  public nsICSSRule,
                                  public nsIDOMCSSImportRule
{
public:
  ImportRule(nsMediaList* aMedia);
private:
  
  ImportRule(const ImportRule& aCopy);
  ~ImportRule();
public:
  NS_DECL_ISUPPORTS

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<nsICSSRule> Clone() const;

  NS_IMETHOD SetURLSpec(const nsString& aURLSpec);
  NS_IMETHOD GetURLSpec(nsString& aURLSpec) const;

  NS_IMETHOD SetMedia(const nsString& aMedia);
  NS_IMETHOD GetMedia(nsString& aMedia) const;

  NS_IMETHOD SetSheet(nsCSSStyleSheet*);

  
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
