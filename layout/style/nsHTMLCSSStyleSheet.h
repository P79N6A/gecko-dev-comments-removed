








#ifndef nsHTMLCSSStyleSheet_h_
#define nsHTMLCSSStyleSheet_h_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

#include "nsDataHashtable.h"
#include "nsIStyleRuleProcessor.h"

struct MiscContainer;

class nsHTMLCSSStyleSheet MOZ_FINAL : public nsIStyleRuleProcessor
{
public:
  nsHTMLCSSStyleSheet();
  ~nsHTMLCSSStyleSheet();

  NS_DECL_ISUPPORTS

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) MOZ_OVERRIDE;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) MOZ_OVERRIDE;
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) MOZ_OVERRIDE;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) MOZ_OVERRIDE;
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE MOZ_OVERRIDE;

  void CacheStyleAttr(const nsAString& aSerialized, MiscContainer* aValue);
  void EvictStyleAttr(const nsAString& aSerialized, MiscContainer* aValue);
  MiscContainer* LookupStyleAttr(const nsAString& aSerialized);

private: 
  nsHTMLCSSStyleSheet(const nsHTMLCSSStyleSheet& aCopy) MOZ_DELETE;
  nsHTMLCSSStyleSheet& operator=(const nsHTMLCSSStyleSheet& aCopy) MOZ_DELETE;

protected:
  nsDataHashtable<nsStringHashKey, MiscContainer*> mCachedStyleAttrs;
};

#endif 
