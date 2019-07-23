






































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
  nsDOMCSSAttributeDeclaration(nsIContent *aContent);
  ~nsDOMCSSAttributeDeclaration();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMCSSAttributeDeclaration)

  
  
  virtual nsresult GetCSSDeclaration(nsCSSDeclaration **aDecl,
                                     PRBool aAllocate);
  virtual nsresult GetCSSParsingEnvironment(nsIURI** aSheetURI,
                                            nsIURI** aBaseURI,
                                            nsIPrincipal** aSheetPrincipal,
                                            nsICSSLoader** aCSSLoader,
                                            nsICSSParser** aCSSParser);
  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent);

  virtual nsISupports *GetParentObject()
  {
    return mContent;
  }

protected:
  virtual nsresult DeclarationChanged();
  
  nsCOMPtr<nsIContent> mContent;
};

#endif 
