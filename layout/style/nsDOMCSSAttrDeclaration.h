







































#ifndef nsDOMCSSAttributeDeclaration_h
#define nsDOMCSSAttributeDeclaration_h

#include "nsDOMCSSDeclaration.h"

#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace css {
class Loader;
}

namespace dom {
class Element;
}
}

class nsDOMCSSAttributeDeclaration : public nsDOMCSSDeclaration,
                                     public nsWrapperCache
{
public:
  typedef mozilla::dom::Element Element;
  nsDOMCSSAttributeDeclaration(Element* aContent
#ifdef MOZ_SMIL
                               , PRBool aIsSMILOverride
#endif 
                               );
  ~nsDOMCSSAttributeDeclaration();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMCSSAttributeDeclaration,
                                           nsICSSDeclaration)

  
  
  virtual mozilla::css::Declaration* GetCSSDeclaration(PRBool aAllocate);
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv);
  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent);

  virtual nsINode* GetParentObject();

protected:
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration* aDecl);
  virtual nsIDocument* DocToUpdate();

  nsRefPtr<Element> mElement;

#ifdef MOZ_SMIL
  



  const PRBool mIsSMILOverride;
#endif 
};

#endif 
