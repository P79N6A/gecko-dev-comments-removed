






#ifndef mozilla_css_ImportRule_h__
#define mozilla_css_ImportRule_h__

#include "mozilla/Attributes.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/css/Rule.h"
#include "nsIDOMCSSImportRule.h"

class nsMediaList;
class nsString;

namespace mozilla {

class CSSStyleSheet;

namespace css {

class ImportRule MOZ_FINAL : public Rule,
                             public nsIDOMCSSImportRule
{
public:
  ImportRule(nsMediaList* aMedia, const nsString& aURLSpec,
             uint32_t aLineNumber, uint32_t aColumnNumber);
private:
  
  ImportRule(const ImportRule& aCopy);
  ~ImportRule();
public:
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(ImportRule, nsIStyleRule)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  DECL_STYLE_RULE_INHERIT

#ifdef HAVE_CPP_AMBIGUITY_RESOLVING_USING
  using Rule::GetStyleSheet; 
#endif

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  
  virtual int32_t GetType() const;
  virtual already_AddRefed<Rule> Clone() const;

  void SetSheet(CSSStyleSheet*);

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSIMPORTRULE

private:
  nsString  mURLSpec;
  nsRefPtr<nsMediaList> mMedia;
  nsRefPtr<CSSStyleSheet> mChildSheet;
};

} 
} 

#endif 
