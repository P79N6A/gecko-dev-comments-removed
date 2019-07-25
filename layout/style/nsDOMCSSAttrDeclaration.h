






































#ifndef nsDOMCSSAttributeDeclaration_h___
#define nsDOMCSSAttributeDeclaration_h___

#include "nsDOMCSSDeclaration.h"

#include "nsString.h"
#include "nsWrapperCache.h"
#include "nsIContent.h"

namespace mozilla {
namespace css {
class Loader;
}
}

class nsDOMCSSAttributeDeclaration : public nsDOMCSSDeclaration,
                                     public nsWrapperCache
{
public:
  nsDOMCSSAttributeDeclaration(nsIContent *aContent
#ifdef MOZ_SMIL
                               , PRBool aIsSMILOverride
#endif 
                               );
  ~nsDOMCSSAttributeDeclaration();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMCSSAttributeDeclaration,
                                           nsICSSDeclaration)

  
  
  virtual nsresult GetCSSDeclaration(mozilla::css::Declaration **aDecl,
                                     PRBool aAllocate);
  virtual nsresult GetCSSParsingEnvironment(nsIURI** aSheetURI,
                                            nsIURI** aBaseURI,
                                            nsIPrincipal** aSheetPrincipal,
                                            mozilla::css::Loader** aCSSLoader);
  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent);

  virtual nsINode *GetParentObject()
  {
    return mContent;
  }

protected:
  virtual nsresult DeclarationChanged();
  virtual nsIDocument* DocToUpdate();

  nsCOMPtr<nsIContent> mContent;

#ifdef MOZ_SMIL
  



  const PRBool mIsSMILOverride;
#endif 
};

#endif 
