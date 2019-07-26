







#ifndef nsITheme_h_
#define nsITheme_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsColor.h"

struct nsRect;
struct nsIntRect;
struct nsIntSize;
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
{ 0x3ca584e6, 0xdcd6, 0x485b, \
  { 0x88, 0x8c, 0xe3, 0x47, 0x3d, 0xe4, 0xd9, 0x58 } }

#define NS_THEMERENDERER_CID \
{ 0x9020805b, 0x14a3, 0x4125, \
  { 0xa5, 0x63, 0x4a, 0x8c, 0x5d, 0xe0, 0xa9, 0xa3 } }











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

  





  NS_IMETHOD GetMinimumWidgetSize(nsRenderingContext* aContext,
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
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITheme, NS_ITHEME_IID)


extern nsresult NS_NewNativeTheme(nsISupports *aOuter, REFNSIID aIID, void **aResult);

#endif
