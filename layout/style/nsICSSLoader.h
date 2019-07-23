






































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
class nsIDocument;
class nsIUnicharInputStream;
class nsICSSLoaderObserver;
class nsMediaList;
class nsICSSImportRule;
class nsIPrincipal;



#define NS_ICSS_LOADER_IID     \
{ 0x33c469dd, 0xaf03, 0x4098, \
 { 0x99, 0x84, 0xb1, 0x3c, 0xee, 0x34, 0xd8, 0x6a } }

typedef void (*nsCSSLoaderCallbackFunc)(nsICSSStyleSheet* aSheet, void *aData, PRBool aDidNotify);

class nsICSSLoader : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_LOADER_IID)

  NS_IMETHOD Init(nsIDocument* aDocument) = 0;
  NS_IMETHOD DropDocumentReference(void) = 0; 

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
                             nsICSSLoaderObserver* aObserver,
                             PRBool* aCompleted,
                             PRBool* aIsAlternate) = 0;

  


















  NS_IMETHOD LoadStyleLink(nsIContent* aElement,
                           nsIURI* aURL, 
                           const nsSubstring& aTitle,
                           const nsSubstring& aMedia,
                           PRBool aHasAlternateRel,
                           nsICSSLoaderObserver* aObserver,
                           PRBool* aIsAlternate) = 0;

  















  NS_IMETHOD LoadChildSheet(nsICSSStyleSheet* aParentSheet,
                            nsIURI* aURL, 
                            nsMediaList* aMedia,
                            nsICSSImportRule* aRule) = 0;

  


























  NS_IMETHOD LoadSheetSync(nsIURI* aURL, PRBool aEnableUnsafeRules,
                           PRBool aUseSystemPrincipal,
                           nsICSSStyleSheet** aSheet) = 0;

  


  nsresult LoadSheetSync(nsIURI* aURL, nsICSSStyleSheet** aSheet) {
    return LoadSheetSync(aURL, PR_FALSE, PR_FALSE, aSheet);
  }

  



















  NS_IMETHOD LoadSheet(nsIURI* aURL,
                       nsIPrincipal* aOriginPrincipal,
                       const nsCString& aCharset,
                       nsICSSLoaderObserver* aObserver,
                       nsICSSStyleSheet** aSheet) = 0;

  



  NS_IMETHOD LoadSheet(nsIURI* aURL,
                       nsIPrincipal* aOriginPrincipal,
                       const nsCString& aCharset,
                       nsICSSLoaderObserver* aObserver) = 0;

  



  NS_IMETHOD Stop(void) = 0;

  



  NS_IMETHOD StopLoadingSheet(nsIURI* aURL) = 0;

  






  NS_IMETHOD GetEnabled(PRBool *aEnabled) = 0;
  NS_IMETHOD SetEnabled(PRBool aEnabled) = 0;

  









  NS_IMETHOD_(PRBool) HasPendingLoads() = 0;

  








  NS_IMETHOD AddObserver(nsICSSLoaderObserver* aObserver) = 0;

  


  NS_IMETHOD_(void) RemoveObserver(nsICSSLoaderObserver* aObserver) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSLoader, NS_ICSS_LOADER_IID)

nsresult 
NS_NewCSSLoader(nsIDocument* aDocument, nsICSSLoader** aLoader);

nsresult 
NS_NewCSSLoader(nsICSSLoader** aLoader);

#endif 
