



#ifndef nsIStyleSheetLinkingElement_h__
#define nsIStyleSheetLinkingElement_h__


#include "nsISupports.h"

class nsICSSLoaderObserver;
class nsIURI;

#define NS_ISTYLESHEETLINKINGELEMENT_IID          \
{ 0xd753c84a, 0x17fd, 0x4d5f, \
 { 0xb2, 0xe9, 0x63, 0x52, 0x8c, 0x87, 0x99, 0x7a } }

class nsIStyleSheet;

namespace mozilla {
class CSSStyleSheet;
} 

class nsIStyleSheetLinkingElement : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLESHEETLINKINGELEMENT_IID)

  






  NS_IMETHOD SetStyleSheet(mozilla::CSSStyleSheet* aStyleSheet) = 0;

  





  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aStyleSheet) = 0;

  





  NS_IMETHOD InitStyleLinkElement(bool aDontLoadStyle) = 0;

  













  NS_IMETHOD UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                              bool *aWillNotify,
                              bool *aIsAlternate) = 0;

  





  NS_IMETHOD SetEnableUpdates(bool aEnableUpdates) = 0;

  




  NS_IMETHOD GetCharset(nsAString& aCharset) = 0;

  






  virtual void OverrideBaseURI(nsIURI* aNewBaseURI) = 0;

  
  
  
  virtual void SetLineNumber(uint32_t aLineNumber) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleSheetLinkingElement,
                              NS_ISTYLESHEETLINKINGELEMENT_IID)

#endif 
