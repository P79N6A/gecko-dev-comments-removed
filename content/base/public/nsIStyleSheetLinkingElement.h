



































#ifndef nsIStyleSheetLinkingElement_h__
#define nsIStyleSheetLinkingElement_h__


#include "nsISupports.h"

class nsIDocument;
class nsICSSLoaderObserver;
class nsIURI;

#define NS_ISTYLESHEETLINKINGELEMENT_IID          \
{ 0xd753c84a, 0x17fd, 0x4d5f, \
 { 0xb2, 0xe9, 0x63, 0x52, 0x8c, 0x87, 0x99, 0x7a } }

class nsIStyleSheet;

class nsIStyleSheetLinkingElement : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLESHEETLINKINGELEMENT_IID)

  






  NS_IMETHOD SetStyleSheet(nsIStyleSheet* aStyleSheet) = 0;

  





  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aStyleSheet) = 0;

  





  NS_IMETHOD InitStyleLinkElement(PRBool aDontLoadStyle) = 0;

  













  NS_IMETHOD UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                              PRBool *aWillNotify,
                              PRBool *aIsAlternate) = 0;

  





  NS_IMETHOD SetEnableUpdates(PRBool aEnableUpdates) = 0;

  




  NS_IMETHOD GetCharset(nsAString& aCharset) = 0;

  






  virtual void OverrideBaseURI(nsIURI* aNewBaseURI) = 0;

  
  
  
  virtual void SetLineNumber(PRUint32 aLineNumber) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleSheetLinkingElement,
                              NS_ISTYLESHEETLINKINGELEMENT_IID)

#endif 
