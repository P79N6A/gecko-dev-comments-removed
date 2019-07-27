







#ifndef nsITheme_h_
#define nsITheme_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsColor.h"

struct nsRect;
struct nsIntRect;
struct nsIntSize;
class nsIntRegion;
struct nsFont;
struct nsIntMargin;
class nsPresContext;
class nsRenderingContext;
class nsDeviceContext;
class nsIFrame;
class nsIContent;
class nsIAtom;
class nsIWidget;



 #define NS_ITHEME_IID     \
{ 0x7a3474d9, 0x3bd6, 0x407c, \
  { 0x86, 0x57, 0xc5, 0xc7, 0x63, 0x36, 0x39, 0xf0 } }

#define NS_THEMERENDERER_CID \
{ 0x0ae05515, 0xcf7a, 0x45a8, \
  { 0x9e, 0x02, 0x65, 0x56, 0xde, 0x76, 0x85, 0xb1 } }











class nsITheme: public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITHEME_IID)

  







  NS_IMETHOD DrawWidgetBackground(nsRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  uint8_t aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aDirtyRect) = 0;

  


  NS_IMETHOD GetWidgetBorder(nsDeviceContext* aContext, 
                             nsIFrame* aFrame,
                             uint8_t aWidgetType,
                             nsIntMargin* aResult)=0;

  








  virtual bool GetWidgetPadding(nsDeviceContext* aContext,
                                  nsIFrame* aFrame,
                                  uint8_t aWidgetType,
                                  nsIntMargin* aResult) = 0;

  













  virtual bool GetWidgetOverflow(nsDeviceContext* aContext,
                                   nsIFrame* aFrame,
                                   uint8_t aWidgetType,
                                    nsRect* aOverflowRect)
  { return false; }

  





  NS_IMETHOD GetMinimumWidgetSize(nsPresContext* aPresContext,
                                  nsIFrame* aFrame,
                                  uint8_t aWidgetType,
                                  nsIntSize* aResult,
                                  bool* aIsOverridable)=0;


  enum Transparency {
    eOpaque = 0,
    eTransparent,
    eUnknownTransparency
  };

  


  virtual Transparency GetWidgetTransparency(nsIFrame* aFrame, uint8_t aWidgetType)
  { return eUnknownTransparency; }

  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, uint8_t aWidgetType, 
                                nsIAtom* aAttribute, bool* aShouldRepaint)=0;

  NS_IMETHOD ThemeChanged()=0;

  virtual bool WidgetAppearanceDependsOnWindowFocus(uint8_t aWidgetType)
  { return false; }

  


  virtual bool ThemeSupportsWidget(nsPresContext* aPresContext,
                                     nsIFrame* aFrame,
                                     uint8_t aWidgetType)=0;

  virtual bool WidgetIsContainer(uint8_t aWidgetType)=0;

  


  virtual bool ThemeDrawsFocusForWidget(uint8_t aWidgetType)=0;
  
  


  virtual bool ThemeNeedsComboboxDropmarker()=0;

  


  virtual bool ShouldHideScrollbars()
  { return false; }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITheme, NS_ITHEME_IID)


extern nsresult NS_NewNativeTheme(nsISupports *aOuter, REFNSIID aIID, void **aResult);

#endif
