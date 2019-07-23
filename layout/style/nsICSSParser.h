






































#ifndef nsICSSParser_h___
#define nsICSSParser_h___

#include "nsISupports.h"
#include "nsAString.h"
#include "nsCSSProperty.h"
#include "nsColor.h"
#include "nsCOMArray.h"

class nsICSSStyleRule;
class nsICSSStyleSheet;
class nsIUnicharInputStream;
class nsIURI;
class nsCSSDeclaration;
class nsICSSLoader;
class nsICSSRule;
class nsMediaList;
class nsIPrincipal;
struct nsCSSSelectorList;

#define NS_ICSS_PARSER_IID    \
{ 0xad4a3778, 0xdae0, 0x4640, \
 { 0xb2, 0x5a, 0x24, 0xff, 0x09, 0xc3, 0x70, 0xef } }


typedef void (* RuleAppendFunc) (nsICSSRule* aRule, void* aData);


class nsICSSParser : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_PARSER_IID)

  
  
  
  NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet) = 0;

  
  NS_IMETHOD SetQuirkMode(PRBool aQuirkMode) = 0;

#ifdef  MOZ_SVG
  
  NS_IMETHOD SetSVGMode(PRBool aSVGMode) = 0;
#endif

  
  NS_IMETHOD SetChildLoader(nsICSSLoader* aChildLoader) = 0;

  















  NS_IMETHOD Parse(nsIUnicharInputStream* aInput,
                   nsIURI*                aSheetURL,
                   nsIURI*                aBaseURI,
                   nsIPrincipal*          aSheetPrincipal,
                   PRUint32               aLineNumber,
                   PRBool                 aAllowUnsafeRules) = 0;

  
  
  
  NS_IMETHOD ParseStyleAttribute(const nsAString&         aAttributeValue,
                                 nsIURI*                  aDocURL,
                                 nsIURI*                  aBaseURL,
                                 nsIPrincipal*            aNodePrincipal,
                                 nsICSSStyleRule**        aResult) = 0;

  NS_IMETHOD ParseAndAppendDeclaration(const nsAString&         aBuffer,
                                       nsIURI*                  aSheetURL,
                                       nsIURI*                  aBaseURL,
                                       nsIPrincipal*            aSheetPrincipal,
                                       nsCSSDeclaration*        aDeclaration,
                                       PRBool                   aParseOnlyOneDecl,
                                       PRBool*                  aChanged,
                                       PRBool                   aClearOldDecl) = 0;

  NS_IMETHOD ParseRule(const nsAString&        aRule,
                       nsIURI*                 aSheetURL,
                       nsIURI*                 aBaseURL,
                       nsIPrincipal*           aSheetPrincipal,
                       nsCOMArray<nsICSSRule>& aResult) = 0;

  NS_IMETHOD ParseProperty(const nsCSSProperty aPropID,
                           const nsAString& aPropValue,
                           nsIURI* aSheetURL,
                           nsIURI* aBaseURL,
                           nsIPrincipal* aSheetPrincipal,
                           nsCSSDeclaration* aDeclaration,
                           PRBool* aChanged) = 0;

  






  NS_IMETHOD ParseMediaList(const nsSubstring& aBuffer,
                            nsIURI* aURL, 
                            PRUint32 aLineNumber, 
                            nsMediaList* aMediaList,
                            PRBool aHTMLMode) = 0;

  








  NS_IMETHOD ParseColorString(const nsSubstring& aBuffer,
                              nsIURI* aURL, 
                              PRUint32 aLineNumber, 
                              nscolor* aColor) = 0;

  



  NS_IMETHOD ParseSelectorString(const nsSubstring& aSelectorString,
                                 nsIURI* aURL, 
                                 PRUint32 aLineNumber, 
                                 nsCSSSelectorList **aSelectorList) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSParser, NS_ICSS_PARSER_IID)

nsresult
NS_NewCSSParser(nsICSSParser** aInstancePtrResult);

#endif 
