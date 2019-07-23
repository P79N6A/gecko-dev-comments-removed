






































#ifndef nsCSSParser_h___
#define nsCSSParser_h___

#include "nsAString.h"
#include "nsCSSProperty.h"
#include "nsColor.h"
#include "nsCOMArray.h"

class nsICSSStyleRule;
class nsICSSLoader;
class nsICSSStyleSheet;
class nsIUnicharInputStream;
class nsIURI;
class nsCSSDeclaration;
class nsICSSLoader;
class nsICSSRule;
class nsMediaList;
class nsIPrincipal;
struct nsCSSSelectorList;



class NS_STACK_CLASS nsCSSParser {
public:
  NS_HIDDEN nsCSSParser(nsICSSLoader* aLoader = nsnull,
                        nsICSSStyleSheet* aSheet = nsnull);
  NS_HIDDEN ~nsCSSParser();

  static void Shutdown();

private:
  
  nsCSSParser(nsCSSParser const&);
  nsCSSParser& operator=(nsCSSParser const&);

public:
  
  
  NS_HIDDEN operator bool() const
  { return !!mImpl; }

  
  
  
  NS_HIDDEN_(nsresult) SetStyleSheet(nsICSSStyleSheet* aSheet);

  
  NS_HIDDEN_(nsresult) SetQuirkMode(PRBool aQuirkMode);

#ifdef  MOZ_SVG
  
  NS_HIDDEN_(nsresult) SetSVGMode(PRBool aSVGMode);
#endif

  
  NS_HIDDEN_(nsresult) SetChildLoader(nsICSSLoader* aChildLoader);

  















  NS_HIDDEN_(nsresult) Parse(nsIUnicharInputStream* aInput,
                             nsIURI*                aSheetURL,
                             nsIURI*                aBaseURI,
                             nsIPrincipal*          aSheetPrincipal,
                             PRUint32               aLineNumber,
                             PRBool                 aAllowUnsafeRules);

  
  
  
  NS_HIDDEN_(nsresult) ParseStyleAttribute(const nsAString&  aAttributeValue,
                                           nsIURI*           aDocURL,
                                           nsIURI*           aBaseURL,
                                           nsIPrincipal*     aNodePrincipal,
                                           nsICSSStyleRule** aResult);

  NS_HIDDEN_(nsresult) ParseAndAppendDeclaration(const nsAString& aBuffer,
                                                 nsIURI* aSheetURL,
                                                 nsIURI* aBaseURL,
                                                 nsIPrincipal* aSheetPrincipal,
                                                 nsCSSDeclaration* aDeclaration,
                                                 PRBool  aParseOnlyOneDecl,
                                                 PRBool* aChanged,
                                                 PRBool  aClearOldDecl);

  NS_HIDDEN_(nsresult) ParseRule(const nsAString&        aRule,
                                 nsIURI*                 aSheetURL,
                                 nsIURI*                 aBaseURL,
                                 nsIPrincipal*           aSheetPrincipal,
                                 nsCOMArray<nsICSSRule>& aResult);

  NS_HIDDEN_(nsresult) ParseProperty(const nsCSSProperty aPropID,
                                     const nsAString&    aPropValue,
                                     nsIURI*             aSheetURL,
                                     nsIURI*             aBaseURL,
                                     nsIPrincipal*       aSheetPrincipal,
                                     nsCSSDeclaration*   aDeclaration,
                                     PRBool*             aChanged);

  







  NS_HIDDEN_(nsresult) ParseMediaList(const nsSubstring& aBuffer,
                                      nsIURI*            aURL,
                                      PRUint32           aLineNumber,
                                      nsMediaList*       aMediaList,
                                      PRBool             aHTMLMode);

  








  NS_HIDDEN_(nsresult) ParseColorString(const nsSubstring& aBuffer,
                                        nsIURI*            aURL,
                                        PRUint32           aLineNumber,
                                        nscolor*           aColor);

  



  NS_HIDDEN_(nsresult) ParseSelectorString(const nsSubstring&  aSelectorString,
                                           nsIURI*             aURL,
                                           PRUint32            aLineNumber,
                                           nsCSSSelectorList** aSelectorList);

protected:
  
  
  
  void* mImpl;
};

#endif 
