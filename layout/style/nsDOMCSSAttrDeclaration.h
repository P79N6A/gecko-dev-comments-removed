






#ifndef nsDOMCSSAttributeDeclaration_h
#define nsDOMCSSAttributeDeclaration_h

#include "mozilla/Attributes.h"
#include "nsDOMCSSDeclaration.h"

#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {
class Element;
}
}

class nsDOMCSSAttributeDeclaration MOZ_FINAL : public nsDOMCSSDeclaration
{
public:
  typedef mozilla::dom::Element Element;
  nsDOMCSSAttributeDeclaration(Element* aContent, bool aIsSMILOverride);
  ~nsDOMCSSAttributeDeclaration();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDOMCSSAttributeDeclaration,
                                                                   nsICSSDeclaration)

  
  
  virtual mozilla::css::Declaration* GetCSSDeclaration(bool aAllocate);
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv) MOZ_OVERRIDE;
  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent) MOZ_OVERRIDE;

  virtual nsINode* GetParentObject() MOZ_OVERRIDE;

protected:
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl) MOZ_OVERRIDE;
  virtual nsIDocument* DocToUpdate() MOZ_OVERRIDE;

  nsRefPtr<Element> mElement;

  



  const bool mIsSMILOverride;
};

#endif 
