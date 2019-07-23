






































#ifndef nsDOMCSSAttributeDeclaration_h___
#define nsDOMCSSAttributeDeclaration_h___

#include "nsIDOMCSSStyleDeclaration.h"
#include "nsDOMCSSDeclaration.h"

#include "nsString.h"

class nsIContent;
class nsICSSLoader;
class nsICSSParser;

class nsDOMCSSAttributeDeclaration : public nsDOMCSSDeclaration
{
public:
  nsDOMCSSAttributeDeclaration(nsIContent *aContent);
  ~nsDOMCSSAttributeDeclaration();

  
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  virtual void DropReference();
  
  
  virtual nsresult GetCSSDeclaration(nsCSSDeclaration **aDecl,
                                     PRBool aAllocate);
  virtual nsresult GetCSSParsingEnvironment(nsIURI** aSheetURI,
                                            nsIURI** aBaseURI,
                                            nsICSSLoader** aCSSLoader,
                                            nsICSSParser** aCSSParser);
  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent);

protected:
  virtual nsresult DeclarationChanged();
  
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  nsIContent *mContent;
};

#endif 
