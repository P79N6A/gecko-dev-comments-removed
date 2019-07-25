






































#ifndef nsCSSParser_h___
#define nsCSSParser_h___

#include "nsAString.h"
#include "nsCSSProperty.h"
#include "nsColor.h"
#include "nsCOMArray.h"

class nsICSSRule;
class nsICSSStyleRule;
class nsCSSStyleSheet;
class nsIPrincipal;
class nsIURI;
class nsIUnicharInputStream;
struct nsCSSSelectorList;
class nsMediaList;

namespace mozilla {
namespace css {
class Declaration;
class Loader;
}
}



class NS_STACK_CLASS nsCSSParser {
public:
  nsCSSParser(mozilla::css::Loader* aLoader = nsnull,
              nsCSSStyleSheet* aSheet = nsnull);
  ~nsCSSParser();

  static void Shutdown();

private:
  
  nsCSSParser(nsCSSParser const&);
  nsCSSParser& operator=(nsCSSParser const&);

public:
  
  
  operator bool() const
  { return !!mImpl; }

  
  
  
  nsresult SetStyleSheet(nsCSSStyleSheet* aSheet);

  
  nsresult SetQuirkMode(PRBool aQuirkMode);

#ifdef  MOZ_SVG
  
  nsresult SetSVGMode(PRBool aSVGMode);
#endif

  
  nsresult SetChildLoader(mozilla::css::Loader* aChildLoader);

  















  nsresult Parse(nsIUnicharInputStream* aInput,
                 nsIURI*                aSheetURL,
                 nsIURI*                aBaseURI,
                 nsIPrincipal*          aSheetPrincipal,
                 PRUint32               aLineNumber,
                 PRBool                 aAllowUnsafeRules);

  
  
  
  nsresult ParseStyleAttribute(const nsAString&  aAttributeValue,
                               nsIURI*           aDocURL,
                               nsIURI*           aBaseURL,
                               nsIPrincipal*     aNodePrincipal,
                               nsICSSStyleRule** aResult);

  nsresult ParseAndAppendDeclaration(const nsAString&  aBuffer,
                                     nsIURI*           aSheetURL,
                                     nsIURI*           aBaseURL,
                                     nsIPrincipal*     aSheetPrincipal,
                                     mozilla::css::Declaration* aDeclaration,
                                     PRBool            aParseOnlyOneDecl,
                                     PRBool*           aChanged,
                                     PRBool            aClearOldDecl);

  nsresult ParseRule(const nsAString&        aRule,
                     nsIURI*                 aSheetURL,
                     nsIURI*                 aBaseURL,
                     nsIPrincipal*           aSheetPrincipal,
                     nsCOMArray<nsICSSRule>& aResult);

  nsresult ParseProperty(const nsCSSProperty aPropID,
                         const nsAString&    aPropValue,
                         nsIURI*             aSheetURL,
                         nsIURI*             aBaseURL,
                         nsIPrincipal*       aSheetPrincipal,
                         mozilla::css::Declaration* aDeclaration,
                         PRBool*             aChanged,
                         PRBool              aIsImportant);

  







  nsresult ParseMediaList(const nsSubstring& aBuffer,
                          nsIURI*            aURL,
                          PRUint32           aLineNumber,
                          nsMediaList*       aMediaList,
                          PRBool             aHTMLMode);

  








  nsresult ParseColorString(const nsSubstring& aBuffer,
                            nsIURI*            aURL,
                            PRUint32           aLineNumber,
                            nscolor*           aColor);

  



  nsresult ParseSelectorString(const nsSubstring&  aSelectorString,
                               nsIURI*             aURL,
                               PRUint32            aLineNumber,
                               nsCSSSelectorList** aSelectorList);

protected:
  
  
  
  void* mImpl;
};

#endif 
