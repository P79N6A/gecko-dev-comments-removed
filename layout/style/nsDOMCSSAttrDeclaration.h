






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

class nsDOMCSSAttributeDeclaration final : public nsDOMCSSDeclaration
{
public:
  typedef mozilla::dom::Element Element;
  nsDOMCSSAttributeDeclaration(Element* aContent, bool aIsSMILOverride);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDOMCSSAttributeDeclaration,
                                                                   nsICSSDeclaration)

  
  
  virtual mozilla::css::Declaration* GetCSSDeclaration(Operation aOperation) override;
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv) override;
  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent) override;

  virtual nsINode* GetParentObject() override;

  NS_IMETHOD SetPropertyValue(const nsCSSProperty aPropID,
                              const nsAString& aValue) override;

protected:
  ~nsDOMCSSAttributeDeclaration();

  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl) override;
  virtual nsIDocument* DocToUpdate() override;

  nsRefPtr<Element> mElement;

  



  const bool mIsSMILOverride;
};

#endif 
