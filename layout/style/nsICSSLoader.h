






































#ifndef nsICSSLoader_h___
#define nsICSSLoader_h___

#include "nsISupports.h"
#include "nsSubstring.h"
#include "nsCompatibility.h"

class nsIAtom;
class nsIURI;
class nsICSSParser;
class nsICSSStyleSheet;
class nsPresContext;
class nsIContent;
class nsIParser;
class nsIDocument;
class nsIUnicharInputStream;
class nsICSSLoaderObserver;
class nsMediaList;
class nsICSSImportRule;



#define NS_ICSS_LOADER_IID     \
{ 0x446711e6, 0xad01, 0x4702, \
 { 0x8a, 0x9b, 0xce, 0x3f, 0x5e, 0x5d, 0x30, 0xf0 } }

typedef void (*nsCSSLoaderCallbackFunc)(nsICSSStyleSheet* aSheet, void *aData, PRBool aDidNotify);

class nsICSSLoader : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_LOADER_IID)

  NS_IMETHOD Init(nsIDocument* aDocument) = 0;
  NS_IMETHOD DropDocumentReference(void) = 0; 

  NS_IMETHOD SetCaseSensitive(PRBool aCaseSensitive) = 0;
  NS_IMETHOD SetCompatibilityMode(nsCompatibility aCompatMode) = 0;
  NS_IMETHOD SetPreferredSheet(const nsAString& aTitle) = 0;
  NS_IMETHOD GetPreferredSheet(nsAString& aTitle) = 0;

  
  NS_IMETHOD GetParserFor(nsICSSStyleSheet* aSheet,
                          nsICSSParser** aParser) = 0;
  NS_IMETHOD RecycleParser(nsICSSParser* aParser) = 0;

  
  
  






















  NS_IMETHOD LoadInlineStyle(nsIContent* aElement,
                             nsIUnicharInputStream* aStream, 
                             PRUint32 aLineNumber,
                             const nsSubstring& aTitle,
                             const nsSubstring& aMedia,
                             nsIParser* aParserToUnblock,
                             nsICSSLoaderObserver* aObserver,
                             PRBool* aCompleted,
                             PRBool* aIsAlternate) = 0;

  





















  NS_IMETHOD LoadStyleLink(nsIContent* aElement,
                           nsIURI* aURL, 
                           const nsSubstring& aTitle,
                           const nsSubstring& aMedia,
                           PRBool aHasAlternateRel,
                           nsIParser* aParserToUnblock,
                           nsICSSLoaderObserver* aObserver,
                           PRBool* aIsAlternate) = 0;

  















  NS_IMETHOD LoadChildSheet(nsICSSStyleSheet* aParentSheet,
                            nsIURI* aURL, 
                            nsMediaList* aMedia,
                            nsICSSImportRule* aRule) = 0;

  
























  NS_IMETHOD LoadSheetSync(nsIURI* aURL, PRBool aEnableUnsafeRules,
                           nsICSSStyleSheet** aSheet) = 0;

  


  nsresult LoadSheetSync(nsIURI* aURL, nsICSSStyleSheet** aSheet) {
    return LoadSheetSync(aURL, PR_FALSE, aSheet);
  }

  











  NS_IMETHOD LoadSheet(nsIURI* aURL, nsICSSLoaderObserver* aObserver,
                       nsICSSStyleSheet** aSheet) = 0;

  



  NS_IMETHOD LoadSheet(nsIURI* aURL, nsICSSLoaderObserver* aObserver) = 0;

  



  NS_IMETHOD Stop(void) = 0;

  



  NS_IMETHOD StopLoadingSheet(nsIURI* aURL) = 0;

  






  NS_IMETHOD GetEnabled(PRBool *aEnabled) = 0;
  NS_IMETHOD SetEnabled(PRBool aEnabled) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSLoader, NS_ICSS_LOADER_IID)

nsresult 
NS_NewCSSLoader(nsIDocument* aDocument, nsICSSLoader** aLoader);

nsresult 
NS_NewCSSLoader(nsICSSLoader** aLoader);

#endif 
