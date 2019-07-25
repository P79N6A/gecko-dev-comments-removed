







































#ifndef nsITheme_h_
#define nsITheme_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsColor.h"

struct nsRect;
struct nsIntSize;
struct nsFont;
struct nsIntMargin;
class nsPresContext;
class nsIRenderingContext;
class nsIDeviceContext;
class nsIFrame;
class nsIContent;
class nsIAtom;



 #define NS_ITHEME_IID     \
{ 0x887e8902, 0xdb6b, 0x41b4, { 0x84, 0x81, 0xa8, 0x0f, 0x49, 0xc5, 0xa9, 0x3a } }


#define NS_THEMERENDERER_CID \
{ 0xd930e29b, 0x6909, 0x44e5, { 0xab, 0x4b, 0xaf, 0x10, 0xd6, 0x92, 0x37, 0x5 } }

enum nsTransparencyMode {
  eTransparencyOpaque = 0,  
  eTransparencyTransparent, 
  eTransparencyGlass        
};











class nsITheme: public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITHEME_IID)
  
  NS_IMETHOD DrawWidgetBackground(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aDirtyRect)=0;

  


  NS_IMETHOD GetWidgetBorder(nsIDeviceContext* aContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType,
                             nsIntMargin* aResult)=0;

  








  virtual PRBool GetWidgetPadding(nsIDeviceContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntMargin* aResult) = 0;

  







  virtual PRBool GetWidgetOverflow(nsIDeviceContext* aContext,
                                   nsIFrame* aFrame,
                                   PRUint8 aWidgetType,
                                    nsRect* aOverflowRect)
  { return PR_FALSE; }

  





  NS_IMETHOD GetMinimumWidgetSize(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntSize* aResult,
                                  PRBool* aIsOverridable)=0;

  virtual nsTransparencyMode GetWidgetTransparency(PRUint8 aWidgetType)=0;

  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                nsIAtom* aAttribute, PRBool* aShouldRepaint)=0;

  NS_IMETHOD ThemeChanged()=0;

  


  virtual PRBool ThemeSupportsWidget(nsPresContext* aPresContext,
                                     nsIFrame* aFrame,
                                     PRUint8 aWidgetType)=0;

  virtual PRBool WidgetIsContainer(PRUint8 aWidgetType)=0;

  


  virtual PRBool ThemeDrawsFocusForWidget(nsPresContext* aPresContext,
                                          nsIFrame* aFrame,
                                          PRUint8 aWidgetType)=0;
  
  


  virtual PRBool ThemeNeedsComboboxDropmarker()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITheme, NS_ITHEME_IID)


extern NS_METHOD NS_NewNativeTheme(nsISupports *aOuter, REFNSIID aIID, void **aResult);

#endif
