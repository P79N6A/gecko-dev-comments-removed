





#include <QStyle>
#include <QPalette>

#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsNativeTheme.h"

class QComboBox;
class QStyleOptionButton;
class QStyleOptionComboBox;
class QRect;
class nsIFrame;
class nsDeviceContext;

class nsNativeThemeQt : private nsNativeTheme,
                        public nsITheme
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD DrawWidgetBackground(nsRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  uint8_t aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aClipRect);

  NS_IMETHOD GetWidgetBorder(nsDeviceContext* aContext,
                             nsIFrame* aFrame,
                             uint8_t aWidgetType,
                             nsIntMargin* aResult);

  NS_IMETHOD GetMinimumWidgetSize(nsRenderingContext* aContext, nsIFrame* aFrame,
                                  uint8_t aWidgetType,
                                  nsIntSize* aResult,
                                  bool* aIsOverridable);

  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, uint8_t aWidgetType,
                                nsIAtom* aAttribute, bool* aShouldRepaint);

  NS_IMETHOD ThemeChanged();

  bool ThemeSupportsWidget(nsPresContext* aPresContext,
                             nsIFrame* aFrame,
                             uint8_t aWidgetType);

  bool WidgetIsContainer(uint8_t aWidgetType);

  virtual NS_HIDDEN_(bool) GetWidgetPadding(nsDeviceContext* aContext,
                                              nsIFrame* aFrame,
                                              uint8_t aWidgetType,
                                              nsIntMargin* aResult);

  NS_IMETHOD_(bool) ThemeDrawsFocusForWidget(nsPresContext* aPresContext,
                                               nsIFrame* aFrame, uint8_t aWidgetType);

  bool ThemeNeedsComboboxDropmarker();

  nsNativeThemeQt();
  virtual ~nsNativeThemeQt();

private:

  inline nsresult DrawWidgetBackground(QPainter *qPainter,
                                       nsRenderingContext* aContext,
                                       nsIFrame* aFrame,
                                       uint8_t aWidgetType,
                                       const nsRect& aRect,
                                       const nsRect& aClipRect);

  void InitButtonStyle(uint8_t widgetType,
                       nsIFrame* aFrame,
                       QRect rect,
                       QStyleOptionButton &opt);

  void InitPlainStyle(uint8_t aWidgetType,
                      nsIFrame* aFrame,
                      QRect rect,
                      QStyleOption &opt,
                      QStyle::State extraFlags = QStyle::State_None);

  void InitComboStyle(uint8_t aWidgetType,
                      nsIFrame* aFrame,
                      QRect rect,
                      QStyleOptionComboBox &opt);

private:

  int32_t mFrameWidth;

  QPalette mNoBackgroundPalette;
};

