




































#ifndef nsNativeThemeCocoa_h_
#define nsNativeThemeCocoa_h_

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsILookAndFeel.h"
#include "nsNativeTheme.h"
#include "gfxASurface.h"

@class CellDrawView;
class nsIDeviceContext;

class nsNativeThemeCocoa : private nsNativeTheme,
                           public nsITheme
{
public:
  nsNativeThemeCocoa();
  virtual ~nsNativeThemeCocoa();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD DrawWidgetBackground(nsRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aDirtyRect);
  NS_IMETHOD GetWidgetBorder(nsIDeviceContext* aContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType,
                             nsIntMargin* aResult);

  virtual PRBool GetWidgetPadding(nsIDeviceContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntMargin* aResult);

  virtual PRBool GetWidgetOverflow(nsIDeviceContext* aContext, nsIFrame* aFrame,
                                   PRUint8 aWidgetType, nsRect* aOverflowRect);

  NS_IMETHOD GetMinimumWidgetSize(nsRenderingContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntSize* aResult, PRBool* aIsOverridable);
  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                nsIAtom* aAttribute, PRBool* aShouldRepaint);
  NS_IMETHOD ThemeChanged();
  PRBool ThemeSupportsWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType);
  PRBool WidgetIsContainer(PRUint8 aWidgetType);
  PRBool ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType);
  PRBool ThemeNeedsComboboxDropmarker();
  virtual Transparency GetWidgetTransparency(nsIFrame* aFrame, PRUint8 aWidgetType);

protected:  

  nsresult GetSystemColor(PRUint8 aWidgetType, nsILookAndFeel::nsColorID& aColorID);
  nsIntMargin RTLAwareMargin(const nsIntMargin& aMargin, nsIFrame* aFrame);

  
  void DrawFrame(CGContextRef context, HIThemeFrameKind inKind,
                 const HIRect& inBoxRect, PRBool inReadOnly,
                 nsEventStates inState);
  void DrawProgress(CGContextRef context, const HIRect& inBoxRect,
                    PRBool inIsIndeterminate, PRBool inIsHorizontal,
                    PRInt32 inValue, PRInt32 inMaxValue, nsIFrame* aFrame);
  void DrawTab(CGContextRef context, HIRect inBoxRect, nsEventStates inState,
               nsIFrame* aFrame);
  void DrawTabPanel(CGContextRef context, const HIRect& inBoxRect, nsIFrame* aFrame);
  void DrawScale(CGContextRef context, const HIRect& inBoxRect,
                 nsEventStates inState, PRBool inDirection,
                 PRBool inIsReverse, PRInt32 inCurrentValue, PRInt32 inMinValue,
                 PRInt32 inMaxValue, nsIFrame* aFrame);
  void DrawCheckboxOrRadio(CGContextRef cgContext, PRBool inCheckbox,
                           const HIRect& inBoxRect, PRBool inSelected,
                           nsEventStates inState, nsIFrame* aFrame);
  void DrawSearchField(CGContextRef cgContext, const HIRect& inBoxRect,
                       nsIFrame* aFrame, nsEventStates inState);
  void DrawPushButton(CGContextRef cgContext, const HIRect& inBoxRect,
                      nsEventStates inState, nsIFrame* aFrame);
  void DrawButton(CGContextRef context, ThemeButtonKind inKind,
                  const HIRect& inBoxRect, PRBool inIsDefault, 
                  ThemeButtonValue inValue, ThemeButtonAdornment inAdornment,
                  nsEventStates inState, nsIFrame* aFrame);
  void DrawDropdown(CGContextRef context, const HIRect& inBoxRect,
                    nsEventStates inState, PRUint8 aWidgetType,
                    nsIFrame* aFrame);
  void DrawSpinButtons(CGContextRef context, ThemeButtonKind inKind,
                       const HIRect& inBoxRect, ThemeDrawState inDrawState,
                       ThemeButtonAdornment inAdornment, nsEventStates inState,
                       nsIFrame* aFrame);
  void DrawUnifiedToolbar(CGContextRef cgContext, const HIRect& inBoxRect,
                          NSWindow* aWindow);
  void DrawStatusBar(CGContextRef cgContext, const HIRect& inBoxRect,
                     nsIFrame *aFrame);
  void DrawResizer(CGContextRef cgContext, const HIRect& aRect, nsIFrame *aFrame);

  
  void DrawScrollbar(CGContextRef aCGContext, const HIRect& aBoxRect, nsIFrame *aFrame);
  void GetScrollbarPressStates (nsIFrame *aFrame, nsEventStates aButtonStates[]);
  void GetScrollbarDrawInfo (HIThemeTrackDrawInfo& aTdi, nsIFrame *aFrame, 
                             const CGSize& aSize, PRBool aShouldGetButtonStates);
  nsIFrame* GetParentScrollbarFrame(nsIFrame *aFrame);

private:
  NSButtonCell* mPushButtonCell;
  NSButtonCell* mRadioButtonCell;
  NSButtonCell* mCheckboxCell;
  NSSearchFieldCell* mSearchFieldCell;
  NSPopUpButtonCell* mDropdownCell;
  NSComboBoxCell* mComboBoxCell;
  CellDrawView* mCellDrawView;
};

#endif 
