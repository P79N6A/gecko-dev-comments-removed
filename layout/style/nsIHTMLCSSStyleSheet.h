









































#ifndef nsIHTMLCSSStyleSheet_h___
#define nsIHTMLCSSStyleSheet_h___

#include "nsIStyleSheet.h"


#define NS_IHTML_CSS_STYLE_SHEET_IID     \
{0xb5cc4ac0, 0xeab6, 0x11d1, {0x80, 0x31, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

class nsIHTMLCSSStyleSheet : public nsIStyleSheet {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTML_CSS_STYLE_SHEET_IID)

  NS_IMETHOD Init(nsIURI* aURL, nsIDocument* aDocument) = 0;
  NS_IMETHOD Reset(nsIURI* aURL) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCSSStyleSheet, NS_IHTML_CSS_STYLE_SHEET_IID)


nsresult
NS_NewHTMLCSSStyleSheet(nsIHTMLCSSStyleSheet** aInstancePtrResult,
                        nsIURI* aURL, nsIDocument* aDocument);

nsresult
NS_NewHTMLCSSStyleSheet(nsIHTMLCSSStyleSheet** aInstancePtrResult);

#endif 
