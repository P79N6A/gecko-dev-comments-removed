






#ifndef mozilla_css_ImportRule_h__
#define mozilla_css_ImportRule_h__

#include "mozilla/Attributes.h"

#include "mozilla/css/Rule.h"
#include "nsIDOMCSSImportRule.h"
#include "nsCSSRules.h"

class nsMediaList;
class nsString;

namespace mozilla {
namespace css {

class ImportRule MOZ_FINAL : public Rule,
                             public nsIDOMCSSImportRule
{
public:
  ImportRule(nsMediaList* aMedia, const nsString& aURLSpec);
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
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  
  virtual int32_t GetType() const;
  virtual already_AddRefed<Rule> Clone() const;

  void SetSheet(nsCSSStyleSheet*);

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSIMPORTRULE

private:
  nsString  mURLSpec;
  nsRefPtr<nsMediaList> mMedia;
  nsRefPtr<nsCSSStyleSheet> mChildSheet;
};

} 
} 

#endif 
