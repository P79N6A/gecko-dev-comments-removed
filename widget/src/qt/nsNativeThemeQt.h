








































#include <QStyle>
#include <QPalette>

#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsNativeTheme.h"
#include "nsIDeviceContext.h"

class QComboBox;
class QStyleOptionButton;
class QStyleOptionFrameV2;
class QStyleOptionComboBox;
class QRect;
class nsIFrame;

class nsNativeThemeQt : private nsNativeTheme,
                        public nsITheme
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD DrawWidgetBackground(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aClipRect);

  NS_IMETHOD GetWidgetBorder(nsIDeviceContext* aContext,
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType,
                             nsIntMargin* aResult);

  NS_IMETHOD GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntSize* aResult,
                                  PRBool* aIsOverridable);

  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType,
                                nsIAtom* aAttribute, PRBool* aShouldRepaint);

  NS_IMETHOD ThemeChanged();

  PRBool ThemeSupportsWidget(nsPresContext* aPresContext,
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType);

  PRBool WidgetIsContainer(PRUint8 aWidgetType);

  virtual NS_HIDDEN_(PRBool) GetWidgetPadding(nsIDeviceContext* aContext,
                                              nsIFrame* aFrame,
                                              PRUint8 aWidgetType,
                                              nsIntMargin* aResult);

  NS_IMETHOD_(PRBool) ThemeDrawsFocusForWidget(nsPresContext* aPresContext,
                                               nsIFrame* aFrame, PRUint8 aWidgetType);

  PRBool ThemeNeedsComboboxDropmarker();

  nsNativeThemeQt();
  virtual ~nsNativeThemeQt();

private:

  inline nsresult DrawWidgetBackground(QPainter *qPainter,
                                       nsIRenderingContext* aContext,
                                       nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       const nsRect& aRect,
                                       const nsRect& aClipRect);

  inline PRInt32 GetAppUnitsPerDevPixel(nsIRenderingContext* aContext){
    nsCOMPtr<nsIDeviceContext> dctx = aContext->GetDeviceContext();
    return dctx->AppUnitsPerDevPixel();
  }

  void InitButtonStyle(PRUint8 widgetType,
                       nsIFrame* aFrame,
                       QRect rect,
                       QStyleOptionButton &opt);

  void InitPlainStyle(PRUint8 aWidgetType,
                      nsIFrame* aFrame,
                      QRect rect,
                      QStyleOption &opt,
                      QStyle::State extraFlags = QStyle::State_None);

  void InitComboStyle(PRUint8 aWidgetType,
                      nsIFrame* aFrame,
                      QRect rect,
                      QStyleOptionComboBox &opt);

private:

  PRInt32 mFrameWidth;

  QPalette mNoBackgroundPalette;
};

