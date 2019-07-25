






































#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsNativeTheme.h"
#include <windows.h>

struct nsIntRect;
struct nsIntSize;

class nsNativeThemeWin : private nsNativeTheme,
                         public nsITheme {
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD DrawWidgetBackground(nsRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aDirtyRect);

  NS_IMETHOD GetWidgetBorder(nsDeviceContext* aContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType,
                             nsIntMargin* aResult);

  virtual PRBool GetWidgetPadding(nsDeviceContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntMargin* aResult);

  virtual PRBool GetWidgetOverflow(nsDeviceContext* aContext,
                                   nsIFrame* aFrame,
                                   PRUint8 aWidgetType,
                                   nsRect* aOverflowRect);

  NS_IMETHOD GetMinimumWidgetSize(nsRenderingContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntSize* aResult,
                                  PRBool* aIsOverridable);

  virtual Transparency GetWidgetTransparency(nsIFrame* aFrame, PRUint8 aWidgetType);

  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                nsIAtom* aAttribute, PRBool* aShouldRepaint);

  NS_IMETHOD ThemeChanged();

  PRBool ThemeSupportsWidget(nsPresContext* aPresContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType);

  PRBool WidgetIsContainer(PRUint8 aWidgetType);

  PRBool ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType);

  PRBool ThemeNeedsComboboxDropmarker();

  nsNativeThemeWin();
  virtual ~nsNativeThemeWin();

protected:
  HANDLE GetTheme(PRUint8 aWidgetType);
  nsresult GetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType,
                                PRInt32& aPart, PRInt32& aState);
  nsresult ClassicGetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType,
                                   PRInt32& aPart, PRInt32& aState, PRBool& aFocused);
  nsresult ClassicDrawWidgetBackground(nsRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aClipRect);
  nsresult ClassicGetWidgetBorder(nsDeviceContext* aContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType,
                             nsIntMargin* aResult);

  nsresult ClassicGetMinimumWidgetSize(nsRenderingContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntSize* aResult,
                                  PRBool* aIsOverridable);

  PRBool ClassicThemeSupportsWidget(nsPresContext* aPresContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType);

  void DrawCheckedRect(HDC hdc, const RECT& rc, PRInt32 fore, PRInt32 back,
                       HBRUSH defaultBack);

  PRUint32 GetWidgetNativeDrawingFlags(PRUint8 aWidgetType);

  PRInt32 StandardGetState(nsIFrame* aFrame, PRUint8 aWidgetType, PRBool wantFocused);

  PRBool IsMenuActive(nsIFrame* aFrame, PRUint8 aWidgetType);
};


extern NS_METHOD NS_NewNativeThemeWin(nsISupports *aOuter, REFNSIID aIID, void **aResult);
