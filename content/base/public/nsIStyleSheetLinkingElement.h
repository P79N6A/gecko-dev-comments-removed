



































#ifndef nsIStyleSheetLinkingElement_h__
#define nsIStyleSheetLinkingElement_h__


#include "nsISupports.h"

class nsIParser;
class nsIDocument;
class nsICSSLoaderObserver;
class nsIURI;

#define NS_ISTYLESHEETLINKINGELEMENT_IID          \
  {0x259f8226, 0x8dd7, 0x11db,                    \
  {0x98, 0x5e, 0x92, 0xb7, 0x56, 0xd8, 0x95, 0x93}}

class nsIStyleSheet;

class nsIStyleSheetLinkingElement : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLESHEETLINKINGELEMENT_IID)

  






  NS_IMETHOD SetStyleSheet(nsIStyleSheet* aStyleSheet) = 0;

  





  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aStyleSheet) = 0;

  








  NS_IMETHOD InitStyleLinkElement(nsIParser *aParser, PRBool aDontLoadStyle) = 0;

  









  NS_IMETHOD UpdateStyleSheet(nsIDocument *aOldDocument,
                              nsICSSLoaderObserver* aObserver,
                              PRBool aForceUpdate = PR_FALSE) = 0;

  





  NS_IMETHOD SetEnableUpdates(PRBool aEnableUpdates) = 0;

  




  NS_IMETHOD GetCharset(nsAString& aCharset) = 0;

  






  virtual void OverrideBaseURI(nsIURI* aNewBaseURI) = 0;

  
  
  
  virtual void SetLineNumber(PRUint32 aLineNumber) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleSheetLinkingElement,
                              NS_ISTYLESHEETLINKINGELEMENT_IID)

#endif 
