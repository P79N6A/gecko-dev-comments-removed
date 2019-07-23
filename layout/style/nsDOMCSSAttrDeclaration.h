






































#ifndef nsDOMCSSAttributeDeclaration_h___
#define nsDOMCSSAttributeDeclaration_h___

#include "nsDOMCSSDeclaration.h"

#include "nsString.h"
#include "nsWrapperCache.h"
#include "nsIContent.h"

class nsICSSLoader;
class nsICSSParser;

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
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMCSSAttributeDeclaration)

  
  
  virtual nsresult GetCSSDeclaration(nsCSSDeclaration **aDecl,
                                     PRBool aAllocate);
  virtual nsresult GetCSSParsingEnvironment(nsIURI** aSheetURI,
                                            nsIURI** aBaseURI,
                                            nsIPrincipal** aSheetPrincipal,
                                            nsICSSLoader** aCSSLoader,
                                            nsICSSParser** aCSSParser);
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
