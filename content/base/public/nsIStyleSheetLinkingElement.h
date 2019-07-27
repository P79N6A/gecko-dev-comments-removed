



#ifndef nsIStyleSheetLinkingElement_h__
#define nsIStyleSheetLinkingElement_h__


#include "nsISupports.h"

class nsICSSLoaderObserver;
class nsIURI;

#define NS_ISTYLESHEETLINKINGELEMENT_IID          \
{ 0xe5855604, 0x8a9a, 0x4181, \
 { 0xbe, 0x41, 0xdd, 0xf7, 0x08, 0x70, 0x3f, 0xbe } }

namespace mozilla {
class CSSStyleSheet;
} 

class nsIStyleSheetLinkingElement : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLESHEETLINKINGELEMENT_IID)

  






  NS_IMETHOD SetStyleSheet(mozilla::CSSStyleSheet* aStyleSheet) = 0;

  




  NS_IMETHOD_(mozilla::CSSStyleSheet*) GetStyleSheet() = 0;

  





  NS_IMETHOD InitStyleLinkElement(bool aDontLoadStyle) = 0;

  















  NS_IMETHOD UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                              bool *aWillNotify,
                              bool *aIsAlternate,
                              bool aForceUpdate = false) = 0;

  





  NS_IMETHOD SetEnableUpdates(bool aEnableUpdates) = 0;

  




  NS_IMETHOD GetCharset(nsAString& aCharset) = 0;

  






  virtual void OverrideBaseURI(nsIURI* aNewBaseURI) = 0;

  
  
  
  virtual void SetLineNumber(uint32_t aLineNumber) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleSheetLinkingElement,
                              NS_ISTYLESHEETLINKINGELEMENT_IID)

#endif 
