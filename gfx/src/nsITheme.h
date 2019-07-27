







#ifndef nsITheme_h_
#define nsITheme_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsColor.h"
#include "Units.h"

struct nsRect;
struct nsIntMargin;
class nsPresContext;
class nsRenderingContext;
class nsDeviceContext;
class nsIFrame;
class nsIAtom;
class nsIWidget;



 #define NS_ITHEME_IID     \
{ 0xa21dd936, 0x5960, 0x46da, \
  { 0xa7, 0x24, 0x7c, 0x11, 0x4e, 0x42, 0x1b, 0x41 } }

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
                                  mozilla::LayoutDeviceIntSize* aResult,
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

  virtual bool NeedToClearBackgroundBehindWidget(uint8_t aWidgetType)
  { return false; }

  virtual bool WidgetProvidesFontSmoothingBackgroundColor(nsIFrame* aFrame,
                                      uint8_t aWidgetType, nscolor* aColor)
  { return false; }

  











  typedef uint8_t ThemeGeometryType;
  enum {
    eThemeGeometryTypeUnknown = 0
  };

  





  virtual ThemeGeometryType ThemeGeometryTypeForWidget(nsIFrame* aFrame,
                                                       uint8_t aWidgetType)
  { return eThemeGeometryTypeUnknown; }

  


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
