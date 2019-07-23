






































#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsNativeTheme.h"
#include <windows.h>

class nsNativeThemeWin : private nsNativeTheme,
                         public nsITheme {
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

  virtual PRBool GetWidgetPadding(nsIDeviceContext* aContext,
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

  PRBool ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType);

  PRBool ThemeNeedsComboboxDropmarker();

  nsNativeThemeWin();
  virtual ~nsNativeThemeWin();

protected:
  void UpdateConfig();
  void CloseData();
  HANDLE GetTheme(PRUint8 aWidgetType);
  nsresult GetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType,
                                PRInt32& aPart, PRInt32& aState);
  nsresult ClassicGetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType,
                                   PRInt32& aPart, PRInt32& aState, PRBool& aFocused);
  nsresult ClassicDrawWidgetBackground(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aClipRect);
  nsresult ClassicGetWidgetBorder(nsIDeviceContext* aContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType,
                             nsMargin* aResult);

  nsresult ClassicGetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsSize* aResult,
                                  PRBool* aIsOverridable);

  PRBool ClassicThemeSupportsWidget(nsPresContext* aPresContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType);

  void DrawCheckedRect(HDC hdc, const RECT& rc, PRInt32 fore, PRInt32 back,
                       HBRUSH defaultBack);

  PRUint32 GetWidgetNativeDrawingFlags(PRUint8 aWidgetType);

private:
  HMODULE mThemeDLL;
  HANDLE mButtonTheme;
  HANDLE mTextFieldTheme;
  HANDLE mTooltipTheme;
  HANDLE mToolbarTheme;
  HANDLE mRebarTheme;
  HANDLE mProgressTheme;
  HANDLE mScrollbarTheme;
  HANDLE mScaleTheme;
  HANDLE mSpinTheme;
  HANDLE mStatusbarTheme;
  HANDLE mTabTheme;
  HANDLE mTreeViewTheme;
  HANDLE mComboBoxTheme;
  HANDLE mHeaderTheme;
  HANDLE mMenuTheme;

  BOOL mFlatMenus;
  OSVERSIONINFO mOsVersion;
};


extern NS_METHOD NS_NewNativeThemeWin(nsISupports *aOuter, REFNSIID aIID, void **aResult);
