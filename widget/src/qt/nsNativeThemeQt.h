








































#include <QStyle>

#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsNativeTheme.h"

class QComboBox;
class QStyleOptionButton;
class QStyleOptionFrameV2;
class QRect;
class nsIFrame;

class nsNativeThemeQt : private nsNativeTheme,
                        public nsITheme
{
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD DrawWidgetBackground(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aClipRect);

  NS_IMETHOD GetWidgetBorder(nsIDeviceContext* aContext,
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType,
                             nsMargin* aResult);

  NS_IMETHOD GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsSize* aResult,
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
                                              nsMargin* aResult);

  NS_IMETHOD_(PRBool) ThemeDrawsFocusForWidget(nsPresContext* aPresContext,
                                               nsIFrame* aFrame, PRUint8 aWidgetType);

  PRBool ThemeNeedsComboboxDropmarker();

  nsNativeThemeQt();
  virtual ~nsNativeThemeQt();

private:

  void EnsuremP2A(nsIRenderingContext* aContext);

  void ButtonStyle(nsIFrame* aFrame,
                   QRect aRect,
                   QStyleOptionButton* aOption,
                   QStyle::State optFlags = QStyle::State_None);

  void FrameStyle(nsIFrame* aFrame,
                  QRect aRect,
                  QStyleOptionFrameV2* aOption,
                  QStyle::State optFlags = QStyle::State_None);

  void PlainStyle(nsIFrame* aFrame,
                  QRect aRect,
                  QStyleOption* aOption,
                  QStyle::State optFlags = QStyle::State_None);

private:

  QComboBox *combo;

  PRInt32 frameWidth;

  PRInt32 mP2A;
};

