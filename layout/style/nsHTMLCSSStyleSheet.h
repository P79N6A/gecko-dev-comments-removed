








#ifndef nsHTMLCSSStyleSheet_h_
#define nsHTMLCSSStyleSheet_h_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

#include "nsDataHashtable.h"
#include "nsIStyleRuleProcessor.h"

class nsRuleWalker;
struct MiscContainer;

namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsHTMLCSSStyleSheet final : public nsIStyleRuleProcessor
{
public:
  nsHTMLCSSStyleSheet();

  NS_DECL_ISUPPORTS

  
  virtual void RulesMatching(ElementRuleProcessorData* aData) override;
  virtual void RulesMatching(PseudoElementRuleProcessorData* aData) override;
  virtual void RulesMatching(AnonBoxRuleProcessorData* aData) override;
#ifdef MOZ_XUL
  virtual void RulesMatching(XULTreeRuleProcessorData* aData) override;
#endif
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint HasStateDependentStyle(PseudoElementStateRuleProcessorData* aData) override;
  virtual bool HasDocumentStateDependentStyle(StateRuleProcessorData* aData) override;
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData) override;
  virtual bool MediumFeaturesChanged(nsPresContext* aPresContext) override;
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE override;

  
  
  void ElementRulesMatching(nsPresContext* aPresContext,
                            mozilla::dom::Element* aElement,
                            nsRuleWalker* aRuleWalker);
  
  
  void PseudoElementRulesMatching(mozilla::dom::Element* aPseudoElement,
                                  nsCSSPseudoElements::Type aPseudoType,
                                  nsRuleWalker* aRuleWalker);

  void CacheStyleAttr(const nsAString& aSerialized, MiscContainer* aValue);
  void EvictStyleAttr(const nsAString& aSerialized, MiscContainer* aValue);
  MiscContainer* LookupStyleAttr(const nsAString& aSerialized);

private: 
  ~nsHTMLCSSStyleSheet();

  nsHTMLCSSStyleSheet(const nsHTMLCSSStyleSheet& aCopy) = delete;
  nsHTMLCSSStyleSheet& operator=(const nsHTMLCSSStyleSheet& aCopy) = delete;

protected:
  nsDataHashtable<nsStringHashKey, MiscContainer*> mCachedStyleAttrs;
};

#endif 
