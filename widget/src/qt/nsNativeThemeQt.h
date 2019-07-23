








































#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsNativeTheme.h"

class QComboBox;

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

  nsNativeThemeQt();
  virtual ~nsNativeThemeQt();

private:
  QComboBox *combo;
  int frameWidth;
};

