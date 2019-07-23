






































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

#define NS_ICSS_PARSER_IID    \
{ 0x2cb34728, 0x0f17, 0x4753, \
  {0x8e, 0xad, 0xec, 0x73, 0xe5, 0x69, 0xcd, 0xcd} }


typedef void (*PR_CALLBACK RuleAppendFunc) (nsICSSRule* aRule, void* aData);


class nsICSSParser : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_PARSER_IID)

  
  
  NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet) = 0;

  
  NS_IMETHOD SetCaseSensitive(PRBool aCaseSensitive) = 0;

  
  NS_IMETHOD SetQuirkMode(PRBool aQuirkMode) = 0;

#ifdef  MOZ_SVG
  
  NS_IMETHOD SetSVGMode(PRBool aSVGMode) = 0;
#endif

  
  NS_IMETHOD SetChildLoader(nsICSSLoader* aChildLoader) = 0;

  



  NS_IMETHOD Parse(nsIUnicharInputStream* aInput,
                   nsIURI*                aSheetURL,
                   nsIURI*                aBaseURI,
                   PRUint32               aLineNumber,
                   PRBool                 aAllowUnsafeRules,
                   nsICSSStyleSheet*&     aResult) = 0;

  
  
  
  NS_IMETHOD ParseStyleAttribute(const nsAString&         aAttributeValue,
                                 nsIURI*                  aDocURL,
                                 nsIURI*                  aBaseURL,
                                 nsICSSStyleRule**        aResult) = 0;

  NS_IMETHOD ParseAndAppendDeclaration(const nsAString&         aBuffer,
                                       nsIURI*                  aSheetURL,
                                       nsIURI*                  aBaseURL,
                                       nsCSSDeclaration*        aDeclaration,
                                       PRBool                   aParseOnlyOneDecl,
                                       PRBool*                  aChanged,
                                       PRBool                   aClearOldDecl) = 0;

  NS_IMETHOD ParseRule(const nsAString&        aRule,
                       nsIURI*                 aSheetURL,
                       nsIURI*                 aBaseURL,
                       nsCOMArray<nsICSSRule>& aResult) = 0;

  NS_IMETHOD ParseProperty(const nsCSSProperty aPropID,
                           const nsAString& aPropValue,
                           nsIURI* aSheetURL,
                           nsIURI* aBaseURL,
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
                              PRBool aHandleAlphaColors,
                              nscolor* aColor) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSParser, NS_ICSS_PARSER_IID)

nsresult
NS_NewCSSParser(nsICSSParser** aInstancePtrResult);

#endif 
