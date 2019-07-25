








































#include <windows.h>
#include "nsNativeThemeWin.h"
#include "nsRenderingContext.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsTransform2D.h"
#include "nsThemeConstants.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsEventStates.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsMenuFrame.h"
#include "nsGkAtoms.h"
#include <malloc.h>
#include "nsWindow.h"
#include "nsIComboboxControlFrame.h"
#include "prinrval.h"
#include "WinUtils.h"

#include "gfxPlatform.h"
#include "gfxContext.h"
#include "gfxMatrix.h"
#include "gfxWindowsPlatform.h"
#include "gfxWindowsSurface.h"

#include "nsUXThemeData.h"
#include "nsUXThemeConstants.h"

using namespace mozilla::widget;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gWindowsLog;
#endif


#define DEFAULT_ANIMATION_FPS 30

NS_IMPL_ISUPPORTS_INHERITED1(nsNativeThemeWin, nsNativeTheme, nsITheme)

static inline bool IsHTMLContent(nsIFrame *frame)
{
  nsIContent* content = frame->GetContent();
  return content && content->IsHTML();
}

static PRInt32 GetTopLevelWindowActiveState(nsIFrame *aFrame)
{
  
  
  nsIWidget* widget = aFrame->GetNearestWidget();
  nsWindow * window = static_cast<nsWindow*>(widget);
  if (!window)
    return mozilla::widget::themeconst::FS_INACTIVE;
  if (widget && !window->IsTopLevelWidget() &&
      !(window = window->GetParentWindow(false)))
    return mozilla::widget::themeconst::FS_INACTIVE;

  if (window->GetWindowHandle() == ::GetActiveWindow())
    return mozilla::widget::themeconst::FS_ACTIVE;
  return mozilla::widget::themeconst::FS_INACTIVE;
}

static PRInt32 GetWindowFrameButtonState(nsIFrame *aFrame, nsEventStates eventState)
{
  if (GetTopLevelWindowActiveState(aFrame) ==
      mozilla::widget::themeconst::FS_INACTIVE) {
    if (eventState.HasState(NS_EVENT_STATE_HOVER))
      return mozilla::widget::themeconst::BS_HOT;
    return mozilla::widget::themeconst::BS_INACTIVE;
  }

  if (eventState.HasState(NS_EVENT_STATE_HOVER)) {
    if (eventState.HasState(NS_EVENT_STATE_ACTIVE))
      return mozilla::widget::themeconst::BS_PUSHED;
    return mozilla::widget::themeconst::BS_HOT;
  }
  return mozilla::widget::themeconst::BS_NORMAL;
}

static PRInt32 GetClassicWindowFrameButtonState(nsEventStates eventState)
{
  if (eventState.HasState(NS_EVENT_STATE_ACTIVE) &&
      eventState.HasState(NS_EVENT_STATE_HOVER))
    return DFCS_BUTTONPUSH|DFCS_PUSHED;
  return DFCS_BUTTONPUSH;
}

static void QueryForButtonData(nsIFrame *aFrame)
{
  if (nsUXThemeData::sTitlebarInfoPopulatedThemed && nsUXThemeData::sTitlebarInfoPopulatedAero)
    return;
  nsIWidget* widget = aFrame->GetNearestWidget();
  nsWindow * window = static_cast<nsWindow*>(widget);
  if (!window)
    return;
  if (!window->IsTopLevelWidget() &&
      !(window = window->GetParentWindow(false)))
    return;

  nsUXThemeData::UpdateTitlebarInfo(window->GetWindowHandle());
}

nsNativeThemeWin::nsNativeThemeWin() {
  
  
  
}

nsNativeThemeWin::~nsNativeThemeWin() {
  nsUXThemeData::Invalidate();
}

static bool IsTopLevelMenu(nsIFrame *aFrame)
{
  bool isTopLevel(false);
  nsMenuFrame *menuFrame = do_QueryFrame(aFrame);
  if (menuFrame) {
    isTopLevel = menuFrame->IsOnMenuBar();
  }
  return isTopLevel;
}

static MARGINS GetCheckboxMargins(HANDLE theme, HDC hdc)
{
    MARGINS checkboxContent = {0};
    GetThemeMargins(theme, hdc, MENU_POPUPCHECK, MCB_NORMAL, TMT_CONTENTMARGINS, NULL, &checkboxContent);
    return checkboxContent;
}
static SIZE GetCheckboxBGSize(HANDLE theme, HDC hdc)
{
    SIZE checkboxSize;
    GetThemePartSize(theme, hdc, MENU_POPUPCHECK, MC_CHECKMARKNORMAL, NULL, TS_TRUE, &checkboxSize);

    MARGINS checkboxMargins = GetCheckboxMargins(theme, hdc);

    int leftMargin = checkboxMargins.cxLeftWidth;
    int rightMargin = checkboxMargins.cxRightWidth;
    int topMargin = checkboxMargins.cyTopHeight;
    int bottomMargin = checkboxMargins.cyBottomHeight;

    int width = leftMargin + checkboxSize.cx + rightMargin;
    int height = topMargin + checkboxSize.cy + bottomMargin;
    SIZE ret;
    ret.cx = width;
    ret.cy = height;
    return ret;
}
static SIZE GetCheckboxBGBounds(HANDLE theme, HDC hdc)
{
    MARGINS checkboxBGSizing = {0};
    MARGINS checkboxBGContent = {0};
    GetThemeMargins(theme, hdc, MENU_POPUPCHECKBACKGROUND, MCB_NORMAL, TMT_SIZINGMARGINS, NULL, &checkboxBGSizing);
    GetThemeMargins(theme, hdc, MENU_POPUPCHECKBACKGROUND, MCB_NORMAL, TMT_CONTENTMARGINS, NULL, &checkboxBGContent);

#define posdx(d) ((d) > 0 ? d : 0)

    int dx = posdx(checkboxBGContent.cxRightWidth - checkboxBGSizing.cxRightWidth) + posdx(checkboxBGContent.cxLeftWidth - checkboxBGSizing.cxLeftWidth);
    int dy = posdx(checkboxBGContent.cyTopHeight - checkboxBGSizing.cyTopHeight) + posdx(checkboxBGContent.cyBottomHeight - checkboxBGSizing.cyBottomHeight);

#undef posdx

    SIZE ret(GetCheckboxBGSize(theme, hdc));
    ret.cx += dx;
    ret.cy += dy;
    return ret;
}
static SIZE GetGutterSize(HANDLE theme, HDC hdc)
{
    SIZE gutterSize;
    GetThemePartSize(theme, hdc, MENU_POPUPGUTTER, 0, NULL, TS_TRUE, &gutterSize);

    SIZE checkboxBGSize(GetCheckboxBGBounds(theme, hdc));

    SIZE itemSize;
    GetThemePartSize(theme, hdc, MENU_POPUPITEM, MPI_NORMAL, NULL, TS_TRUE, &itemSize);

    int width = NS_MAX(itemSize.cx, checkboxBGSize.cx + gutterSize.cx);
    int height = NS_MAX(itemSize.cy, checkboxBGSize.cy);
    SIZE ret;
    ret.cx = width;
    ret.cy = height;
    return ret;
}


























static bool
RenderThemedAnimationFrame(gfxContext* aCtx,
                           gfxWindowsNativeDrawing* aNative,
                           HANDLE aTheme, HDC aHdc,
                           int aPartsList[], int aPartCount,
                           int aBaseState, int aAlphaState,
                           double aAlpha,
                           const gfxRect& aDRect, const gfxRect& aDDirtyRect,
                           const RECT& aWRect, const RECT& aWDirtyRect)
{
  NS_ASSERTION(aPartCount > 0, "Bad parts array.");
  NS_ASSERTION(aCtx, "Bad context.");
  NS_ASSERTION(aNative, "Bad native pointer.");

#if 0
  printf("rect:(%d %d %d %d) dirty:(%d %d %d %d) alpha=%f\n",
  aWRect.left, aWRect.top, aWRect.right, aWRect.bottom,
  aWDirtyRect.left, aWDirtyRect.top, aWDirtyRect.right, aWDirtyRect.bottom,
  aAlpha);
#endif

  
  
  bool backBufferInUse = aNative->IsDoublePass();

  nsRefPtr<gfxContext> paintCtx;
  if (backBufferInUse) {
    
    
    
    nsRefPtr<gfxASurface> currentSurf = aNative->GetCurrentSurface();
    NS_ENSURE_TRUE(currentSurf, false);

    
    paintCtx = new gfxContext(currentSurf);
    NS_ENSURE_TRUE(paintCtx, false);
  } else {
    
    paintCtx = aCtx;
  }

  int width = aWDirtyRect.right - aWDirtyRect.left;
  int height = aWDirtyRect.bottom - aWDirtyRect.top;

  RECT surfaceDrawRect = { 0, 0, width, height }; 

  
  nsRefPtr<gfxWindowsSurface> surfBase =
    new gfxWindowsSurface(gfxIntSize(width, height),
                          gfxASurface::ImageFormatRGB24);
  NS_ENSURE_TRUE(surfBase, false);

  
  
  if (backBufferInUse) {
    
    if (!aNative->IsSecondPass()) {
      FillRect(surfBase->GetDC(), &surfaceDrawRect,
               (HBRUSH)GetStockObject(BLACK_BRUSH));
    } else {
      FillRect(surfBase->GetDC(), &surfaceDrawRect,
               (HBRUSH)GetStockObject(WHITE_BRUSH));
    }
  } else {
    
    BitBlt(surfBase->GetDC(), 0, 0, width, height, aHdc, aWDirtyRect.left,
           aWDirtyRect.top, SRCCOPY);
  }

  
  for (int idx = 0; idx < aPartCount; idx++) {
    DrawThemeBackground(aTheme, surfBase->GetDC(), aPartsList[idx],
                        aBaseState, &surfaceDrawRect, &surfaceDrawRect);
  }

  
  nsRefPtr<gfxWindowsSurface> surfAlpha =
    new gfxWindowsSurface(gfxIntSize(width, height),
                          gfxASurface::ImageFormatRGB24);
  NS_ENSURE_TRUE(surfAlpha, false);

  if (backBufferInUse) {
    if (!aNative->IsSecondPass()) {
      FillRect(surfAlpha->GetDC(), &surfaceDrawRect,
               (HBRUSH)GetStockObject(BLACK_BRUSH));
    } else {
      FillRect(surfAlpha->GetDC(), &surfaceDrawRect,
               (HBRUSH)GetStockObject(WHITE_BRUSH));
    }
  } else {
    BitBlt(surfAlpha->GetDC(), 0, 0, width, height, aHdc, aWDirtyRect.left,
           aWDirtyRect.top, SRCCOPY);
  }

  
  for (int idx = 0; idx < aPartCount; idx++) {
    DrawThemeBackground(aTheme, surfAlpha->GetDC(), aPartsList[idx],
                        aAlphaState, &surfaceDrawRect, &surfaceDrawRect);
 }

  
  nsRefPtr<gfxImageSurface> imageBase = surfBase->GetAsImageSurface();
  nsRefPtr<gfxImageSurface> imageAlpha = surfAlpha->GetAsImageSurface();
  NS_ENSURE_TRUE(imageBase, false);
  NS_ENSURE_TRUE(imageAlpha, false);

  gfxContext::GraphicsOperator currentOp = paintCtx->CurrentOperator();
  paintCtx->Save();
  paintCtx->ResetClip();
  if (!backBufferInUse) {
    
    
    
    
    gfxRect roundedRect = aDDirtyRect;
    roundedRect.Round();
    paintCtx->Clip(roundedRect);
    paintCtx->Translate(roundedRect.TopLeft());
  }
  paintCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
  paintCtx->SetSource(imageBase);
  paintCtx->Paint();
  paintCtx->SetOperator(gfxContext::OPERATOR_OVER);
  paintCtx->SetSource(imageAlpha);
  paintCtx->Paint(aAlpha);
  paintCtx->Restore();
  paintCtx->SetOperator(currentOp);

  return true;
}













void
nsNativeThemeWin::QueueAnimation(gfxWindowsNativeDrawing* aNativeDrawing,
                                 nsIContent* aContent, FadeState aDirection,
                                 DWORD aDuration, PRUint32 aUserValue)
{
  NS_ASSERTION(aNativeDrawing, "bad draw pointer");
  NS_ASSERTION(aContent, "bad content pointer");
  NS_ASSERTION((aDirection == FADE_IN ||
                aDirection == FADE_OUT), "bad direction");
  
  
  
  
  
  if (!aNativeDrawing->IsDoublePass() || aNativeDrawing->IsSecondPass())
    QueueAnimatedContentRefreshForFade(aContent, aDirection,
      DEFAULT_ANIMATION_FPS, aDuration, aUserValue);
}





static int
GetScrollbarButtonBasicState(int aState)
{
  switch(aState) {
    case ABS_UPHOT:
    case ABS_DOWNHOT:
    case ABS_LEFTHOT:
    case ABS_RIGHTHOT:
      return TS_HOVER;
    break;
    case ABS_UPPRESSED:
    case ABS_DOWNPRESSED:
    case ABS_LEFTPRESSED:
    case ABS_RIGHTPRESSED:
      return TS_ACTIVE;
    break;
    case ABS_UPDISABLED:
    case ABS_DOWNDISABLED:
    case ABS_LEFTDISABLED:
    case ABS_RIGHTDISABLED:
      return TS_DISABLED;
    break;
    case ABS_UPHOVER:
    case ABS_DOWNHOVER:
    case ABS_LEFTHOVER:
    case ABS_RIGHTHOVER:
      return TS_FOCUSED;
    break;
    default:
      return TS_NORMAL;
  }
}





void
nsNativeThemeWin::GetScrollbarButtonProperFadeStates(int aBasicState,
                                                     nsIContent* aContent,
                                                     int aWidgetType,
                                                     int& aStartState,
                                                     int& aFinalState)
{
  if (aBasicState == TS_FOCUSED) {
    
    switch(aWidgetType) {
      case NS_THEME_SCROLLBAR_BUTTON_UP:
        aStartState = ABS_UPNORMAL;
        aFinalState = ABS_UPHOVER;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_DOWN:
        aStartState = ABS_DOWNNORMAL;
        aFinalState = ABS_DOWNHOVER;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_LEFT:
        aStartState = ABS_LEFTNORMAL;
        aFinalState = ABS_LEFTHOVER;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
        aStartState = ABS_RIGHTNORMAL;
        aFinalState = ABS_RIGHTHOVER;
      break;
    }
  } else if (aBasicState == TS_HOVER) {
    
    switch(aWidgetType) {
      case NS_THEME_SCROLLBAR_BUTTON_UP:
        aStartState = ABS_UPNORMAL;
        aFinalState = ABS_UPHOT;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_DOWN:
        aStartState = ABS_DOWNNORMAL;
        aFinalState = ABS_DOWNHOT;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_LEFT:
        aStartState = ABS_LEFTNORMAL;
        aFinalState = ABS_LEFTHOT;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
        aStartState = ABS_RIGHTNORMAL;
        aFinalState = ABS_RIGHTHOT;
      break;
    }
  } else if (aBasicState == TS_NORMAL) {
    switch(aWidgetType) {
      case NS_THEME_SCROLLBAR_BUTTON_UP:
        aStartState = ABS_UPNORMAL;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_DOWN:
        aStartState = ABS_DOWNNORMAL;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_LEFT:
        aStartState = ABS_LEFTNORMAL;
      break;
      case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
        aStartState = ABS_RIGHTNORMAL;
      break;
    }
    aFinalState = GetFadeUserData(aContent);
  } else {
    NS_NOTREACHED("Unexpected state for Win7/Vista scroll bar buttons");
  }
}













static DWORD
GetThemedTransitionDuration(HANDLE aTheme, int aPartId,
                            int aStateIdFrom, int aStateIdTo)
{
  DWORD duration = 0;
  
  if (!nsUXThemeData::getThemeTransitionDuration)
    return 0;
  nsUXThemeData::getThemeTransitionDuration(aTheme, aPartId,
                                            aStateIdFrom,
                                            aStateIdTo,
                                            TMT_TRANSITIONDURATIONS,
                                            &duration);
  return duration;
}

static HRESULT DrawThemeBGRTLAware(HANDLE theme, HDC hdc, int part, int state,
                                   const RECT *widgetRect, const RECT *clipRect,
                                   bool isRTL)
{
  





















  if (isRTL) {
    HGDIOBJ hObj = GetCurrentObject(hdc, OBJ_BITMAP);
    BITMAP bitmap;
    POINT vpOrg;

    if (hObj &&
        GetObject(hObj, sizeof(bitmap), &bitmap) &&
        GetViewportOrgEx(hdc, &vpOrg))
    {
      RECT newWRect(*widgetRect);
      newWRect.left = bitmap.bmWidth - (widgetRect->right + 2*vpOrg.x);
      newWRect.right = bitmap.bmWidth - (widgetRect->left + 2*vpOrg.x);

      RECT newCRect;
      RECT *newCRectPtr = NULL;

      if (clipRect) {
        newCRect.top = clipRect->top;
        newCRect.bottom = clipRect->bottom;
        newCRect.left = bitmap.bmWidth - (clipRect->right + 2*vpOrg.x);
        newCRect.right = bitmap.bmWidth - (clipRect->left + 2*vpOrg.x);
        newCRectPtr = &newCRect;
      }

      SetLayout(hdc, LAYOUT_RTL);
      HRESULT hr = DrawThemeBackground(theme, hdc, part, state, &newWRect, newCRectPtr);
      SetLayout(hdc, 0);

      if (hr == S_OK)
        return hr;
    }
  }

  
  return DrawThemeBackground(theme, hdc, part, state, widgetRect, clipRect);
}




































enum CaptionDesktopTheme {
  CAPTION_CLASSIC = 0,
  CAPTION_BASIC,
  CAPTION_XPTHEME,
};

enum CaptionButton {
  CAPTIONBUTTON_MINIMIZE = 0,
  CAPTIONBUTTON_RESTORE,
  CAPTIONBUTTON_CLOSE,
};

struct CaptionButtonPadding {
  RECT hotPadding[3];
};


static CaptionButtonPadding buttonData[3] = {
  { 
    { { 1, 2, 0, 1 }, { 0, 2, 1, 1 }, { 1, 2, 2, 1 } }
  },
  { 
    { { 1, 2, 0, 2 }, { 0, 2, 1, 2 }, { 1, 2, 2, 2 } }
  },
  { 
    { { 0, 2, 0, 2 }, { 0, 2, 1, 2 }, { 1, 2, 2, 2 } }
  }
};







static const PRInt32 kProgressHorizontalVistaOverlaySize = 120;

static const PRInt32 kProgressHorizontalXPOverlaySize = 55;

static const PRInt32 kProgressVerticalOverlaySize = 45;

static const PRInt32 kProgressVerticalIndeterminateOverlaySize = 60;

static const PRInt32 kProgressClassicOverlaySize = 40;

static const double kProgressDeterminedVistaSpeed = 0.225;

static const double kProgressIndeterminateSpeed = 0.175;

static const double kProgressClassicIndeterminateSpeed = 0.0875;

static const PRInt32 kProgressIndeterminateDelay = 500;

static const PRInt32 kProgressDeterminedVistaDelay = 1000;


static void AddPaddingRect(nsIntSize* aSize, CaptionButton button) {
  if (!aSize)
    return;
  RECT offset;
  if (!IsAppThemed())
    offset = buttonData[CAPTION_CLASSIC].hotPadding[button];
  else if (WinUtils::GetWindowsVersion() == WinUtils::WINXP_VERSION)
    offset = buttonData[CAPTION_XPTHEME].hotPadding[button];
  else
    offset = buttonData[CAPTION_BASIC].hotPadding[button];
  aSize->width += offset.left + offset.right;
  aSize->height += offset.top + offset.bottom;
}



static void OffsetBackgroundRect(RECT& rect, CaptionButton button) {
  RECT offset;
  if (!IsAppThemed())
    offset = buttonData[CAPTION_CLASSIC].hotPadding[button];
  else if (WinUtils::GetWindowsVersion() == WinUtils::WINXP_VERSION)
    offset = buttonData[CAPTION_XPTHEME].hotPadding[button];
  else
    offset = buttonData[CAPTION_BASIC].hotPadding[button];
  rect.left += offset.left;
  rect.top += offset.top;
  rect.right -= offset.right;
  rect.bottom -= offset.bottom;
}

HANDLE
nsNativeThemeWin::GetTheme(PRUint8 aWidgetType)
{ 
  if (WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION) {
    
    
    
    
    if (aWidgetType == NS_THEME_DROPDOWN)
      aWidgetType = NS_THEME_TEXTFIELD;
  }

  switch (aWidgetType) {
    case NS_THEME_BUTTON:
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX:
    case NS_THEME_GROUPBOX:
      return nsUXThemeData::GetTheme(eUXButton);
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
      return nsUXThemeData::GetTheme(eUXEdit);
    case NS_THEME_TOOLTIP:
      
      return WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION ?
        NULL : nsUXThemeData::GetTheme(eUXTooltip);
    case NS_THEME_TOOLBOX:
      return nsUXThemeData::GetTheme(eUXRebar);
    case NS_THEME_WIN_MEDIA_TOOLBOX:
      return nsUXThemeData::GetTheme(eUXMediaRebar);
    case NS_THEME_WIN_COMMUNICATIONS_TOOLBOX:
      return nsUXThemeData::GetTheme(eUXCommunicationsRebar);
    case NS_THEME_WIN_BROWSER_TAB_BAR_TOOLBOX:
      return nsUXThemeData::GetTheme(eUXBrowserTabBarRebar);
    case NS_THEME_TOOLBAR:
    case NS_THEME_TOOLBAR_BUTTON:
    case NS_THEME_TOOLBAR_SEPARATOR:
      return nsUXThemeData::GetTheme(eUXToolbar);
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
      return nsUXThemeData::GetTheme(eUXProgress);
    case NS_THEME_TAB:
    case NS_THEME_TAB_PANEL:
    case NS_THEME_TAB_PANELS:
      return nsUXThemeData::GetTheme(eUXTab);
    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_SMALL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
      return nsUXThemeData::GetTheme(eUXScrollbar);
    case NS_THEME_SCALE_HORIZONTAL:
    case NS_THEME_SCALE_VERTICAL:
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL:
      return nsUXThemeData::GetTheme(eUXTrackbar);
    case NS_THEME_SPINNER_UP_BUTTON:
    case NS_THEME_SPINNER_DOWN_BUTTON:
      return nsUXThemeData::GetTheme(eUXSpin);
    case NS_THEME_STATUSBAR:
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_RESIZER:
      return nsUXThemeData::GetTheme(eUXStatus);
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_BUTTON:
      return nsUXThemeData::GetTheme(eUXCombobox);
    case NS_THEME_TREEVIEW_HEADER_CELL:
    case NS_THEME_TREEVIEW_HEADER_SORTARROW:
      return nsUXThemeData::GetTheme(eUXHeader);
    case NS_THEME_LISTBOX:
    case NS_THEME_LISTBOX_LISTITEM:
    case NS_THEME_TREEVIEW:
    case NS_THEME_TREEVIEW_TWISTY_OPEN:
    case NS_THEME_TREEVIEW_TREEITEM:
      return nsUXThemeData::GetTheme(eUXListview);
    case NS_THEME_MENUBAR:
    case NS_THEME_MENUPOPUP:
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM:
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
    case NS_THEME_MENUSEPARATOR:
    case NS_THEME_MENUARROW:
    case NS_THEME_MENUIMAGE:
    case NS_THEME_MENUITEMTEXT:
      return nsUXThemeData::GetTheme(eUXMenu);
    case NS_THEME_WINDOW_TITLEBAR:
    case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
    case NS_THEME_WINDOW_FRAME_LEFT:
    case NS_THEME_WINDOW_FRAME_RIGHT:
    case NS_THEME_WINDOW_FRAME_BOTTOM:
    case NS_THEME_WINDOW_BUTTON_CLOSE:
    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
    case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
    case NS_THEME_WINDOW_BUTTON_RESTORE:
    case NS_THEME_WINDOW_BUTTON_BOX:
    case NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED:
    case NS_THEME_WIN_GLASS:
    case NS_THEME_WIN_BORDERLESS_GLASS:
      return nsUXThemeData::GetTheme(eUXWindowFrame);
  }
  return NULL;
}

PRInt32
nsNativeThemeWin::StandardGetState(nsIFrame* aFrame, PRUint8 aWidgetType,
                                   bool wantFocused)
{
  nsEventStates eventState = GetContentState(aFrame, aWidgetType);
  if (eventState.HasAllStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE))
    return TS_ACTIVE;
  if (eventState.HasState(NS_EVENT_STATE_HOVER))
    return TS_HOVER;
  if (wantFocused && eventState.HasState(NS_EVENT_STATE_FOCUS))
    return TS_FOCUSED;

  return TS_NORMAL;
}

bool
nsNativeThemeWin::IsMenuActive(nsIFrame *aFrame, PRUint8 aWidgetType)
{
  nsIContent* content = aFrame->GetContent();
  if (content->IsXUL() &&
      content->NodeInfo()->Equals(nsGkAtoms::richlistitem))
    return CheckBooleanAttr(aFrame, nsGkAtoms::selected);

  return CheckBooleanAttr(aFrame, nsGkAtoms::menuactive);
}








nsresult 
nsNativeThemeWin::GetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                       PRInt32& aPart, PRInt32& aState)
{
  if (WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION) {
    
    if (aWidgetType == NS_THEME_DROPDOWN)
      aWidgetType = NS_THEME_TEXTFIELD;
  }

  switch (aWidgetType) {
    case NS_THEME_BUTTON: {
      aPart = BP_BUTTON;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }

      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      if (IsDisabled(aFrame, eventState)) {
        aState = TS_DISABLED;
        return NS_OK;
      } else if (IsOpenButton(aFrame) ||
                 IsCheckedButton(aFrame)) {
        aState = TS_ACTIVE;
        return NS_OK;
      }

      aState = StandardGetState(aFrame, aWidgetType, true);
      
      
      
      if (aState == TS_NORMAL && IsDefaultButton(aFrame))
        aState = TS_FOCUSED;
      return NS_OK;
    }
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO: {
      bool isCheckbox = (aWidgetType == NS_THEME_CHECKBOX);
      aPart = isCheckbox ? BP_CHECKBOX : BP_RADIO;

      enum InputState {
        UNCHECKED = 0, CHECKED, INDETERMINATE
      };
      InputState inputState = UNCHECKED;
      bool isXULCheckboxRadio = false;

      if (!aFrame) {
        aState = TS_NORMAL;
      } else {
        if (GetCheckedOrSelected(aFrame, !isCheckbox)) {
          inputState = CHECKED;
        } if (isCheckbox && GetIndeterminate(aFrame)) {
          inputState = INDETERMINATE;
        }

        nsEventStates eventState = GetContentState(isXULCheckboxRadio ? aFrame->GetParent()
                                                                      : aFrame,
                                             aWidgetType);
        if (IsDisabled(aFrame, eventState)) {
          aState = TS_DISABLED;
        } else {
          aState = StandardGetState(aFrame, aWidgetType, false);
        }
      }

      
      aState += inputState * 4;
      return NS_OK;
    }
    case NS_THEME_GROUPBOX: {
      aPart = BP_GROUPBOX;
      aState = TS_NORMAL;
      
      
      return NS_OK;
    }
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE: {
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
        







        aPart = TFP_EDITBORDER_NOSCROLL;

        if (!aFrame) {
          aState = TFS_EDITBORDER_NORMAL;
        } else if (IsDisabled(aFrame, eventState)) {
          aState = TFS_EDITBORDER_DISABLED;
        } else if (IsReadOnly(aFrame)) {
          
          aState = TFS_EDITBORDER_NORMAL;
        } else {
          nsIContent* content = aFrame->GetContent();

          


          if (content && content->IsXUL() && IsFocused(aFrame))
            aState = TFS_EDITBORDER_FOCUSED;
          else if (eventState.HasAtLeastOneOfStates(NS_EVENT_STATE_ACTIVE | NS_EVENT_STATE_FOCUS))
            aState = TFS_EDITBORDER_FOCUSED;
          else if (eventState.HasState(NS_EVENT_STATE_HOVER))
            aState = TFS_EDITBORDER_HOVER;
          else
            aState = TFS_EDITBORDER_NORMAL;
        }
      } else {
        aPart = TFP_TEXTFIELD;
        
        if (!aFrame)
          aState = TS_NORMAL;
        else if (IsDisabled(aFrame, eventState))
          aState = TS_DISABLED;
        else if (IsReadOnly(aFrame))
          aState = TFS_READONLY;
        else
          aState = StandardGetState(aFrame, aWidgetType, true);
      }

      return NS_OK;
    }
    case NS_THEME_TOOLTIP: {
      aPart = TTP_STANDARD;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR: {
      aPart = IsVerticalProgress(aFrame) ? PP_BARVERT : PP_BAR;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR_CHUNK: {
      nsIFrame* stateFrame = aFrame->GetParent();
      nsEventStates eventStates = GetContentState(stateFrame, aWidgetType);

      if (IsIndeterminateProgress(stateFrame, eventStates)) {
        
        
        aPart = -1;
      } else if (IsVerticalProgress(stateFrame)) {
        aPart = WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION ?
          PP_FILLVERT : PP_CHUNKVERT;
      } else {
        aPart = WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION ?
          PP_FILL : PP_CHUNK;
      }

      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR_VERTICAL: {
      aPart = PP_BARVERT;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL: {
      aPart = PP_CHUNKVERT;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TOOLBAR_BUTTON: {
      aPart = BP_BUTTON;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }

      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      if (IsDisabled(aFrame, eventState)) {
        aState = TS_DISABLED;
        return NS_OK;
      }
      if (IsOpenButton(aFrame)) {
        aState = TS_ACTIVE;
        return NS_OK;
      }

      if (eventState.HasAllStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE))
        aState = TS_ACTIVE;
      else if (eventState.HasState(NS_EVENT_STATE_HOVER)) {
        if (IsCheckedButton(aFrame))
          aState = TB_HOVER_CHECKED;
        else
          aState = TS_HOVER;
      }
      else {
        if (IsCheckedButton(aFrame))
          aState = TB_CHECKED;
        else
          aState = TS_NORMAL;
      }
     
      return NS_OK;
    }
    case NS_THEME_TOOLBAR_SEPARATOR: {
      aPart = TP_SEPARATOR;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT: {
      aPart = SP_BUTTON;
      aState = (aWidgetType - NS_THEME_SCROLLBAR_BUTTON_UP)*4;
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      if (!aFrame)
        aState += TS_NORMAL;
      else if (IsDisabled(aFrame, eventState))
        aState += TS_DISABLED;
      else {
        nsIFrame *parent = aFrame->GetParent();
        nsEventStates parentState = GetContentState(parent, parent->GetStyleDisplay()->mAppearance);
        if (eventState.HasAllStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE))
          aState += TS_ACTIVE;
        else if (eventState.HasState(NS_EVENT_STATE_HOVER))
          aState += TS_HOVER;
        else if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION &&
                 parentState.HasState(NS_EVENT_STATE_HOVER))
          aState = (aWidgetType - NS_THEME_SCROLLBAR_BUTTON_UP) + SP_BUTTON_IMPLICIT_HOVER_BASE;
        else
          aState += TS_NORMAL;
      }
      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL: {
      aPart = (aWidgetType == NS_THEME_SCROLLBAR_TRACK_HORIZONTAL) ?
              SP_TRACKSTARTHOR : SP_TRACKSTARTVERT;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL: {
      aPart = (aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL) ?
              SP_THUMBHOR : SP_THUMBVERT;
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      if (!aFrame)
        aState = TS_NORMAL;
      else if (IsDisabled(aFrame, eventState))
        aState = TS_DISABLED;
      else {
        if (eventState.HasState(NS_EVENT_STATE_ACTIVE)) 
                                                        
                                                        
          aState = TS_ACTIVE;
        else if (eventState.HasState(NS_EVENT_STATE_HOVER))
          aState = TS_HOVER;
        else 
          aState = TS_NORMAL;
      }
      return NS_OK;
    }
    case NS_THEME_SCALE_HORIZONTAL:
    case NS_THEME_SCALE_VERTICAL: {
      aPart = (aWidgetType == NS_THEME_SCALE_HORIZONTAL) ?
              TKP_TRACK : TKP_TRACKVERT;

      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL: {
      aPart = (aWidgetType == NS_THEME_SCALE_THUMB_HORIZONTAL) ?
              TKP_THUMB : TKP_THUMBVERT;
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      if (!aFrame)
        aState = TS_NORMAL;
      else if (IsDisabled(aFrame, eventState)) {
        aState = TKP_DISABLED;
      }
      else {
        if (eventState.HasState(NS_EVENT_STATE_ACTIVE)) 
                                                        
                                                        
          aState = TS_ACTIVE;
        else if (eventState.HasState(NS_EVENT_STATE_FOCUS))
          aState = TKP_FOCUSED;
        else if (eventState.HasState(NS_EVENT_STATE_HOVER))
          aState = TS_HOVER;
        else
          aState = TS_NORMAL;
      }
      return NS_OK;
    }
    case NS_THEME_SPINNER_UP_BUTTON:
    case NS_THEME_SPINNER_DOWN_BUTTON: {
      aPart = (aWidgetType == NS_THEME_SPINNER_UP_BUTTON) ?
              SPNP_UP : SPNP_DOWN;
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      if (!aFrame)
        aState = TS_NORMAL;
      else if (IsDisabled(aFrame, eventState))
        aState = TS_DISABLED;
      else
        aState = StandardGetState(aFrame, aWidgetType, false);
      return NS_OK;    
    }
    case NS_THEME_TOOLBOX:
    case NS_THEME_WIN_MEDIA_TOOLBOX:
    case NS_THEME_WIN_COMMUNICATIONS_TOOLBOX:
    case NS_THEME_WIN_BROWSER_TAB_BAR_TOOLBOX:
    case NS_THEME_STATUSBAR:
    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_SMALL: {
      aState = 0;
      if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
        
        aPart = RP_BACKGROUND;
      } else {
        
        
        aPart = 0;
      }
      return NS_OK;
    }
    case NS_THEME_TOOLBAR: {
      
      
      
      aPart = -1;
      aState = 0;
      if (aFrame) {
        nsIContent* content = aFrame->GetContent();
        nsIContent* parent = content->GetParent();
        
        if (parent && parent->GetFirstChild() == content) {
          aState = 1;
        }
      }
      return NS_OK;
    }
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_RESIZER: {
      aPart = (aWidgetType - NS_THEME_STATUSBAR_PANEL) + 1;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TREEVIEW:
    case NS_THEME_LISTBOX: {
      aPart = TREEVIEW_BODY;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TAB_PANELS: {
      aPart = TABP_PANELS;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TAB_PANEL: {
      aPart = TABP_PANEL;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_TAB: {
      aPart = TABP_TAB;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }

      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      if (IsDisabled(aFrame, eventState)) {
        aState = TS_DISABLED;
        return NS_OK;
      }

      if (IsSelectedTab(aFrame)) {
        aPart = TABP_TAB_SELECTED;
        aState = TS_ACTIVE; 
      }
      else
        aState = StandardGetState(aFrame, aWidgetType, true);
      
      return NS_OK;
    }
    case NS_THEME_TREEVIEW_HEADER_SORTARROW: {
      
      aPart = 4;
      aState = 1;
      return NS_OK;
    }
    case NS_THEME_TREEVIEW_HEADER_CELL: {
      aPart = 1;
      if (!aFrame) {
        aState = TS_NORMAL;
        return NS_OK;
      }
      
      aState = StandardGetState(aFrame, aWidgetType, true);
      
      return NS_OK;
    }
    case NS_THEME_DROPDOWN: {
      nsIContent* content = aFrame->GetContent();
      bool isHTML = content && content->IsHTML();
      bool useDropBorder = isHTML || IsMenuListEditable(aFrame);
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      


      if (useDropBorder)
        aPart = CBP_DROPBORDER;
      else
        aPart = CBP_DROPFRAME;

      if (IsDisabled(aFrame, eventState)) {
        aState = TS_DISABLED;
      } else if (IsReadOnly(aFrame)) {
        aState = TS_NORMAL;
      } else if (IsOpenButton(aFrame)) {
        aState = TS_ACTIVE;
      } else {
        if (useDropBorder && (eventState.HasState(NS_EVENT_STATE_FOCUS) || IsFocused(aFrame)))
          aState = TS_ACTIVE;
        else if (eventState.HasAllStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE))
          aState = TS_ACTIVE;
        else if (eventState.HasState(NS_EVENT_STATE_HOVER))
          aState = TS_HOVER;
        else
          aState = TS_NORMAL;
      }

      return NS_OK;
    }
    case NS_THEME_DROPDOWN_BUTTON: {
      bool isHTML = IsHTMLContent(aFrame);
      nsIFrame* parentFrame = aFrame->GetParent();
      bool isMenulist = !isHTML && parentFrame->GetType() == nsGkAtoms::menuFrame;
      bool isOpen = false;

      
      if (isHTML || isMenulist)
        aFrame = parentFrame;

      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      aPart = WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION ?
        CBP_DROPMARKER_VISTA : CBP_DROPMARKER;

      
      
      
      if (isHTML && IsWidgetStyled(aFrame->PresContext(), aFrame, NS_THEME_DROPDOWN))
        aPart = CBP_DROPMARKER;

      if (IsDisabled(aFrame, eventState)) {
        aState = TS_DISABLED;
        return NS_OK;
      }

      if (isHTML) {
        nsIComboboxControlFrame* ccf = do_QueryFrame(aFrame);
        isOpen = (ccf && ccf->IsDroppedDown());
      }
      else
        isOpen = IsOpenButton(aFrame);

      if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
        if (isHTML || IsMenuListEditable(aFrame)) {
          if (isOpen) {
            






            aState = TS_HOVER;
            return NS_OK;
          }
        } else {
          




          aState = TS_NORMAL;
          return NS_OK;
        }
      }

      aState = TS_NORMAL;

      
      if (eventState.HasState(NS_EVENT_STATE_ACTIVE)) {
        if (isOpen && (isHTML || isMenulist)) {
          
          
          return NS_OK;
        }
        aState = TS_ACTIVE;
      }
      else if (eventState.HasState(NS_EVENT_STATE_HOVER)) {
        
        
        if (isOpen) {
          
          
          return NS_OK;
        }
        aState = TS_HOVER;
      }
      return NS_OK;
    }
    case NS_THEME_MENUPOPUP: {
      aPart = MENU_POPUPBACKGROUND;
      aState = MB_ACTIVE;
      return NS_OK;
    }
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM: 
    case NS_THEME_RADIOMENUITEM: {
      bool isTopLevel = false;
      bool isOpen = false;
      bool isHover = false;
      nsMenuFrame *menuFrame = do_QueryFrame(aFrame);
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      isTopLevel = IsTopLevelMenu(aFrame);

      if (menuFrame)
        isOpen = menuFrame->IsOpen();

      isHover = IsMenuActive(aFrame, aWidgetType);

      if (isTopLevel) {
        aPart = MENU_BARITEM;

        if (isOpen)
          aState = MBI_PUSHED;
        else if (isHover)
          aState = MBI_HOT;
        else
          aState = MBI_NORMAL;

        
        if (IsDisabled(aFrame, eventState))
          aState += 3;
      } else {
        aPart = MENU_POPUPITEM;

        if (isHover)
          aState = MPI_HOT;
        else
          aState = MPI_NORMAL;

        
        if (IsDisabled(aFrame, eventState))
          aState += 2;
      }

      return NS_OK;
    }
    case NS_THEME_MENUSEPARATOR:
      aPart = MENU_POPUPSEPARATOR;
      aState = 0;
      return NS_OK;
    case NS_THEME_MENUARROW:
      {
        aPart = MENU_POPUPSUBMENU;
        nsEventStates eventState = GetContentState(aFrame, aWidgetType);
        aState = IsDisabled(aFrame, eventState) ? MSM_DISABLED : MSM_NORMAL;
        return NS_OK;
      }
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
      {
        bool isChecked;
        nsEventStates eventState = GetContentState(aFrame, aWidgetType);

        
        isChecked = CheckBooleanAttr(aFrame, nsGkAtoms::checked);

        aPart = MENU_POPUPCHECK;
        aState = MC_CHECKMARKNORMAL;

        
        if (aWidgetType == NS_THEME_MENURADIO)
          aState += 2;

        
        if (IsDisabled(aFrame, eventState))
          aState += 1;

        return NS_OK;
      }
    case NS_THEME_MENUITEMTEXT:
    case NS_THEME_MENUIMAGE:
      aPart = -1;
      aState = 0;
      return NS_OK;

    case NS_THEME_WINDOW_TITLEBAR:
      aPart = mozilla::widget::themeconst::WP_CAPTION;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
      aPart = mozilla::widget::themeconst::WP_MAXCAPTION;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_FRAME_LEFT:
      aPart = mozilla::widget::themeconst::WP_FRAMELEFT;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_FRAME_RIGHT:
      aPart = mozilla::widget::themeconst::WP_FRAMERIGHT;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_FRAME_BOTTOM:
      aPart = mozilla::widget::themeconst::WP_FRAMEBOTTOM;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_CLOSE:
      aPart = mozilla::widget::themeconst::WP_CLOSEBUTTON;
      aState = GetWindowFrameButtonState(aFrame, GetContentState(aFrame, aWidgetType));
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
      aPart = mozilla::widget::themeconst::WP_MINBUTTON;
      aState = GetWindowFrameButtonState(aFrame, GetContentState(aFrame, aWidgetType));
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
      aPart = mozilla::widget::themeconst::WP_MAXBUTTON;
      aState = GetWindowFrameButtonState(aFrame, GetContentState(aFrame, aWidgetType));
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_RESTORE:
      aPart = mozilla::widget::themeconst::WP_RESTOREBUTTON;
      aState = GetWindowFrameButtonState(aFrame, GetContentState(aFrame, aWidgetType));
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_BOX:
    case NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED:
    case NS_THEME_WIN_GLASS:
    case NS_THEME_WIN_BORDERLESS_GLASS:
      aPart = -1;
      aState = 0;
      return NS_OK;
  }

  aPart = 0;
  aState = 0;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNativeThemeWin::DrawWidgetBackground(nsRenderingContext* aContext,
                                       nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       const nsRect& aRect,
                                       const nsRect& aDirtyRect)
{
  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return ClassicDrawWidgetBackground(aContext, aFrame, aWidgetType, aRect, aDirtyRect); 

  
  if (nsUXThemeData::CheckForCompositor()) {
    switch (aWidgetType) {
      case NS_THEME_WINDOW_TITLEBAR:
      case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
      case NS_THEME_WINDOW_FRAME_LEFT:
      case NS_THEME_WINDOW_FRAME_RIGHT:
      case NS_THEME_WINDOW_FRAME_BOTTOM:
        
        
        return NS_OK;
      break;
      case NS_THEME_WINDOW_BUTTON_CLOSE:
      case NS_THEME_WINDOW_BUTTON_MINIMIZE:
      case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
      case NS_THEME_WINDOW_BUTTON_RESTORE:
        
        
        
        return NS_OK;
      break;
      case NS_THEME_WIN_GLASS:
      case NS_THEME_WIN_BORDERLESS_GLASS:
        
        return NS_OK;
      break;
    }
  }

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  gfxFloat p2a = gfxFloat(aContext->AppUnitsPerDevPixel());
  RECT widgetRect;
  RECT clipRect;
  gfxRect tr(aRect.x, aRect.y, aRect.width, aRect.height),
          dr(aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);

  tr.ScaleInverse(p2a);
  dr.ScaleInverse(p2a);

  
  if (aWidgetType == NS_THEME_DROPDOWN_BUTTON &&
      part == CBP_DROPMARKER_VISTA && IsHTMLContent(aFrame))
  {
    tr.y -= 1.0;
    tr.width += 1.0;
    tr.height += 2.0;

    dr.y -= 1.0;
    dr.width += 1.0;
    dr.height += 2.0;

    if (IsFrameRTL(aFrame)) {
      tr.x -= 1.0;
      dr.x -= 1.0;
    }
  }

  nsRefPtr<gfxContext> ctx = aContext->ThebesContext();

  gfxWindowsNativeDrawing nativeDrawing(ctx, dr, GetWidgetNativeDrawingFlags(aWidgetType));

RENDER_AGAIN:

  HDC hdc = nativeDrawing.BeginNativeDrawing();
  if (!hdc)
    return NS_ERROR_FAILURE;

  nativeDrawing.TransformToNativeRect(tr, widgetRect);
  nativeDrawing.TransformToNativeRect(dr, clipRect);

#if 0
  {
    PR_LOG(gWindowsLog, PR_LOG_ERROR,
           (stderr, "xform: %f %f %f %f [%f %f]\n", m.xx, m.yx, m.xy, m.yy, 
            m.x0, m.y0));
    PR_LOG(gWindowsLog, PR_LOG_ERROR,
           (stderr, "tr: [%d %d %d %d]\ndr: [%d %d %d %d]\noff: [%f %f]\n",
            tr.x, tr.y, tr.width, tr.height, dr.x, dr.y, dr.width, dr.height,
            offset.x, offset.y));
  }
#endif

  if (aWidgetType == NS_THEME_WINDOW_TITLEBAR) {
    
    
    widgetRect.left -= GetSystemMetrics(SM_CXFRAME);
    widgetRect.right += GetSystemMetrics(SM_CXFRAME);
  } else if (aWidgetType == NS_THEME_WINDOW_TITLEBAR_MAXIMIZED) {
    
    
    
    widgetRect.top += GetSystemMetrics(SM_CYFRAME);
  } else if (aWidgetType == NS_THEME_TAB) {
    
    
    bool isLeft = IsLeftToSelectedTab(aFrame);
    bool isRight = !isLeft && IsRightToSelectedTab(aFrame);

    if (isLeft || isRight) {
      
      
      
      PRInt32 edgeSize = 2;
    
      
      
      
      if (isLeft)
        
        widgetRect.right += edgeSize;
      else
        
        widgetRect.left -= edgeSize;
    }
  }
  else if (aWidgetType == NS_THEME_WINDOW_BUTTON_MINIMIZE) {
    OffsetBackgroundRect(widgetRect, CAPTIONBUTTON_MINIMIZE);
  }
  else if (aWidgetType == NS_THEME_WINDOW_BUTTON_MAXIMIZE ||
           aWidgetType == NS_THEME_WINDOW_BUTTON_RESTORE) {
    OffsetBackgroundRect(widgetRect, CAPTIONBUTTON_RESTORE);
  }
  else if (aWidgetType == NS_THEME_WINDOW_BUTTON_CLOSE) {
    OffsetBackgroundRect(widgetRect, CAPTIONBUTTON_CLOSE);
  }

  
  
  
  if (aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
      aWidgetType == NS_THEME_SCALE_VERTICAL) {
    RECT contentRect;
    GetThemeBackgroundContentRect(theme, hdc, part, state, &widgetRect, &contentRect);

    SIZE siz;
    GetThemePartSize(theme, hdc, part, state, &widgetRect, TS_TRUE, &siz);

    if (aWidgetType == NS_THEME_SCALE_HORIZONTAL) {
      PRInt32 adjustment = (contentRect.bottom - contentRect.top - siz.cy) / 2 + 1;
      contentRect.top += adjustment;
      contentRect.bottom -= adjustment;
    }
    else {
      PRInt32 adjustment = (contentRect.right - contentRect.left - siz.cx) / 2 + 1;
      
      
      contentRect.left += adjustment - 1;
      contentRect.right -= adjustment;
    }

    DrawThemeBackground(theme, hdc, part, state, &contentRect, &clipRect);
  }
  else if (aWidgetType == NS_THEME_MENUCHECKBOX || aWidgetType == NS_THEME_MENURADIO)
  {
      bool isChecked = false;
      isChecked = CheckBooleanAttr(aFrame, nsGkAtoms::checked);

      if (isChecked)
      {
        int bgState = MCB_NORMAL;
        nsEventStates eventState = GetContentState(aFrame, aWidgetType);

        
        if (IsDisabled(aFrame, eventState))
          bgState += 1;

        SIZE checkboxBGSize(GetCheckboxBGSize(theme, hdc));

        RECT checkBGRect = widgetRect;
        if (IsFrameRTL(aFrame)) {
          checkBGRect.left = checkBGRect.right-checkboxBGSize.cx;
        } else {
          checkBGRect.right = checkBGRect.left+checkboxBGSize.cx;
        }

        
        checkBGRect.top += (checkBGRect.bottom - checkBGRect.top)/2 - checkboxBGSize.cy/2;
        checkBGRect.bottom = checkBGRect.top + checkboxBGSize.cy;

        DrawThemeBackground(theme, hdc, MENU_POPUPCHECKBACKGROUND, bgState, &checkBGRect, &clipRect);

        MARGINS checkMargins = GetCheckboxMargins(theme, hdc);
        RECT checkRect = checkBGRect;
        checkRect.left += checkMargins.cxLeftWidth;
        checkRect.right -= checkMargins.cxRightWidth;
        checkRect.top += checkMargins.cyTopHeight;
        checkRect.bottom -= checkMargins.cyBottomHeight;
        DrawThemeBackground(theme, hdc, MENU_POPUPCHECK, state, &checkRect, &clipRect);
      }
  }
  else if (aWidgetType == NS_THEME_MENUPOPUP)
  {
    DrawThemeBackground(theme, hdc, MENU_POPUPBORDERS,  0, &widgetRect, &clipRect);
    SIZE borderSize;
    GetThemePartSize(theme, hdc, MENU_POPUPBORDERS, 0, NULL, TS_TRUE, &borderSize);

    RECT bgRect = widgetRect;
    bgRect.top += borderSize.cy;
    bgRect.bottom -= borderSize.cy;
    bgRect.left += borderSize.cx;
    bgRect.right -= borderSize.cx;

    DrawThemeBackground(theme, hdc, MENU_POPUPBACKGROUND,  0, &bgRect, &clipRect);

    SIZE gutterSize(GetGutterSize(theme, hdc));

    RECT gutterRect;
    gutterRect.top = bgRect.top;
    gutterRect.bottom = bgRect.bottom;
    if (IsFrameRTL(aFrame)) {
      gutterRect.right = bgRect.right;
      gutterRect.left = gutterRect.right-gutterSize.cx;
    } else {
      gutterRect.left = bgRect.left;
      gutterRect.right = gutterRect.left+gutterSize.cx;
    }

    DrawThemeBGRTLAware(theme, hdc, MENU_POPUPGUTTER,  0,
                        &gutterRect, &clipRect, IsFrameRTL(aFrame));
  }
  else if (aWidgetType == NS_THEME_MENUSEPARATOR)
  {
    SIZE gutterSize(GetGutterSize(theme,hdc));

    RECT sepRect = widgetRect;
    if (IsFrameRTL(aFrame))
      sepRect.right -= gutterSize.cx;
    else
      sepRect.left += gutterSize.cx;

    DrawThemeBackground(theme, hdc, MENU_POPUPSEPARATOR,  0, &sepRect, &clipRect);
  }
  
  else if (aWidgetType == NS_THEME_MENUARROW ||
           aWidgetType == NS_THEME_RESIZER ||
           aWidgetType == NS_THEME_DROPDOWN_BUTTON)
  {
    DrawThemeBGRTLAware(theme, hdc, part, state,
                        &widgetRect, &clipRect, IsFrameRTL(aFrame));
  }
  else if (aWidgetType == NS_THEME_SCROLLBAR_THUMB_VERTICAL ||
           aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL) {
    
    SIZE gripSize;
    MARGINS thumbMgns;
    int gripPart = (aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL) ?
                    SP_GRIPPERHOR : SP_GRIPPERVERT;
    bool drawGripper = 
      (GetThemePartSize(theme, hdc, gripPart, state, NULL,
                        TS_TRUE, &gripSize) == S_OK &&
       GetThemeMargins(theme, hdc, part, state,
                       TMT_CONTENTMARGINS, NULL,
                       &thumbMgns) == S_OK &&
       gripSize.cx + thumbMgns.cxLeftWidth + thumbMgns.cxRightWidth <=
         widgetRect.right - widgetRect.left &&
       gripSize.cy + thumbMgns.cyTopHeight + thumbMgns.cyBottomHeight <=
         widgetRect.bottom - widgetRect.top);

    nsIContent* content = nsnull;
    if (aFrame) {
      content = aFrame->GetContent();
    }
    FadeState fState = GetFadeState(content);
    DWORD duration = GetThemedTransitionDuration(theme, part,
                                                 TS_NORMAL,
                                                 TS_HOVER);
    
    if (WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION ||
        state == TS_ACTIVE || state == TS_DISABLED ||
        (state == TS_NORMAL && fState == FADE_NOTACTIVE) ||
        !aFrame || !duration || !content) {
      
      
      
      DrawThemeBackground(theme, hdc, part, state, &widgetRect, &clipRect);
      if (drawGripper)
        DrawThemeBackground(theme, hdc, gripPart, state, &widgetRect,
                            &clipRect);
      if (state == TS_ACTIVE) {
        
        
        FinishFadeIn(content);
      }
    } else {
      int partsList[2];
      partsList[0] = part;
      partsList[1] = gripPart;
      int partCount = (drawGripper ? 2 : 1);
      if (RenderThemedAnimationFrame(ctx, &nativeDrawing, theme, hdc,
                                     partsList, partCount,
                                     TS_NORMAL, TS_HOVER,
                                     GetFadeAlpha(content),
                                     tr, dr, widgetRect, clipRect)) {
        QueueAnimation(&nativeDrawing, content,
                       (state == TS_HOVER ? FADE_IN : FADE_OUT), duration);
      }
    }
  }
  else if (aWidgetType == NS_THEME_SCROLLBAR_BUTTON_UP ||
           aWidgetType == NS_THEME_SCROLLBAR_BUTTON_DOWN ||
           aWidgetType == NS_THEME_SCROLLBAR_BUTTON_LEFT ||
           aWidgetType == NS_THEME_SCROLLBAR_BUTTON_RIGHT) {
    int basicState = GetScrollbarButtonBasicState(state);
    nsIContent* content = nsnull;
    if (aFrame) {
      content = aFrame->GetContent();
    }
    FadeState fState = GetFadeState(content);
    
    DWORD duration = GetThemedTransitionDuration(theme,
                                                 SBP_ARROWBTN,
                                                 ABS_UPNORMAL,
                                                 ABS_UPHOT);
    if (WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION ||
        basicState == TS_ACTIVE || basicState == TS_DISABLED ||
        (basicState == TS_NORMAL && fState == FADE_NOTACTIVE) ||
        !aFrame || !duration || !content) {
      DrawThemeBackground(theme, hdc, part, state, &widgetRect, &clipRect);
    } else {
      int partsList[1];
      partsList[0] = part;
      int startState, finalState;
      GetScrollbarButtonProperFadeStates(basicState, content, aWidgetType,
                                         startState, finalState);
      if (RenderThemedAnimationFrame(ctx, &nativeDrawing, theme, hdc,
                                     partsList, 1,
                                     startState, finalState,
                                     GetFadeAlpha(content),
                                     tr, dr, widgetRect, clipRect)) {
        QueueAnimation(&nativeDrawing, content,
                       (basicState == TS_NORMAL ? FADE_OUT : FADE_IN),
                       duration, finalState);
      }
    }
  }
  else if (aWidgetType == NS_THEME_BUTTON) {
    nsIContent* content = nsnull;
    if (aFrame) {
      content = aFrame->GetContent();
    }
    FadeState fState = GetFadeState(content);
    DWORD duration = GetThemedTransitionDuration(theme,
                                                 BP_PUSHBUTTON,
                                                 PBS_NORMAL,
                                                 PBS_HOT);
    if (WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION ||
        state == PBS_PRESSED || state == PBS_DISABLED ||
        ((state == PBS_NORMAL || state == PBS_DEFAULTED) &&
         fState == FADE_NOTACTIVE) || !aFrame || !duration || !content) {
      DrawThemeBackground(theme, hdc, part, state, &widgetRect, &clipRect);
    } else {
      
      
      
      
      int startState, finalState;
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);
      if (eventState.HasState(NS_EVENT_STATE_FOCUS) ||
          IsDefaultButton(aFrame)) {
        startState = PBS_DEFAULTED;
        finalState = PBS_DEFAULTED_ANIMATING;
      } else {
        startState = PBS_NORMAL;
        finalState = PBS_HOT;
      }

      int partsList[1];
      partsList[0] = part;
      if (RenderThemedAnimationFrame(ctx, &nativeDrawing, theme, hdc,
                                     partsList, 1,
                                     startState, finalState,
                                     GetFadeAlpha(content),
                                     tr, dr, widgetRect, clipRect)) {
        QueueAnimation(&nativeDrawing, content,
                       ((state == PBS_NORMAL || state == PBS_DEFAULTED) ?
                         FADE_OUT : FADE_IN), duration);
      }
    }
  }
  
  
  else if (part >= 0) {
    DrawThemeBackground(theme, hdc, part, state, &widgetRect, &clipRect);
  }

  
  
  if (((aWidgetType == NS_THEME_CHECKBOX || aWidgetType == NS_THEME_RADIO) &&
        aFrame->GetContent()->IsHTML()) ||
      aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
      aWidgetType == NS_THEME_SCALE_VERTICAL) {
      nsEventStates contentState = GetContentState(aFrame, aWidgetType);

      if (contentState.HasState(NS_EVENT_STATE_FOCUS)) {
        POINT vpOrg;
        HPEN hPen = nsnull;

        PRUint8 id = SaveDC(hdc);

        ::SelectClipRgn(hdc, NULL);
        ::GetViewportOrgEx(hdc, &vpOrg);
        ::SetBrushOrgEx(hdc, vpOrg.x + widgetRect.left, vpOrg.y + widgetRect.top, NULL);

        
        
        if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION &&
            aWidgetType == NS_THEME_CHECKBOX) {
          LOGBRUSH lb;
          lb.lbStyle = BS_SOLID;
          lb.lbColor = RGB(255,255,255);
          lb.lbHatch = 0;

          hPen = ::ExtCreatePen(PS_COSMETIC|PS_ALTERNATE, 1, &lb, 0, NULL);
          ::SelectObject(hdc, hPen);

          
          if (contentState.HasState(NS_EVENT_STATE_ACTIVE)) {
            ::MoveToEx(hdc, widgetRect.left, widgetRect.bottom-1, NULL);
            ::LineTo(hdc, widgetRect.left, widgetRect.top);
            ::LineTo(hdc, widgetRect.right-1, widgetRect.top);
          }

          
          ::MoveToEx(hdc, widgetRect.right-1, widgetRect.top, NULL);
          ::LineTo(hdc, widgetRect.right-1, widgetRect.bottom-1);
          ::LineTo(hdc, widgetRect.left, widgetRect.bottom-1);
        } else {
          ::SetTextColor(hdc, 0);
          ::DrawFocusRect(hdc, &widgetRect);
        }
        ::RestoreDC(hdc, id);
        if (hPen) {
          ::DeleteObject(hPen);
        }
      }
  }
  else if (aWidgetType == NS_THEME_TOOLBAR && state == 0) {
    
    
    theme = GetTheme(NS_THEME_TOOLBOX);
    if (!theme)
      return NS_ERROR_FAILURE;

    widgetRect.bottom = widgetRect.top + TB_SEPARATOR_HEIGHT;
    DrawThemeEdge(theme, hdc, RP_BAND, 0, &widgetRect, EDGE_ETCHED, BF_TOP, NULL);
  }
  else if ((aWidgetType == NS_THEME_WINDOW_BUTTON_BOX ||
            aWidgetType == NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED) &&
            nsUXThemeData::CheckForCompositor())
  {
    
    
    ctx->Save();
    ctx->ResetClip();
    ctx->Translate(dr.TopLeft());

    
    gfxRect buttonbox1(0.0, 0.0, dr.Width(), dr.Height() - 2.0);
    gfxRect buttonbox2(1.0, dr.Height() - 2.0, dr.Width() - 1.0, 1.0);
    gfxRect buttonbox3(2.0, dr.Height() - 1.0, dr.Width() - 3.0, 1.0);

    gfxContext::GraphicsOperator currentOp = ctx->CurrentOperator();
    ctx->SetOperator(gfxContext::OPERATOR_CLEAR);

   
   
   
    ctx->NewPath();
    ctx->Rectangle(buttonbox1, true);
    ctx->Fill();

    ctx->NewPath();
    ctx->Rectangle(buttonbox2, true);
    ctx->Fill();

    ctx->NewPath();
    ctx->Rectangle(buttonbox3, true);
    ctx->Fill();

    ctx->Restore();
    ctx->SetOperator(currentOp);
  } else if (aWidgetType == NS_THEME_PROGRESSBAR_CHUNK) {
    









    nsIFrame* stateFrame = aFrame->GetParent();
    nsEventStates eventStates = GetContentState(stateFrame, aWidgetType);
    bool indeterminate = IsIndeterminateProgress(stateFrame, eventStates);
    bool vertical = IsVerticalProgress(stateFrame);

    if (indeterminate ||
        WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
      if (!QueueAnimatedContentForRefresh(aFrame->GetContent(),
                                          DEFAULT_ANIMATION_FPS)) {
        NS_WARNING("unable to animate progress widget!");
      }

      





      PRInt32 overlaySize;
      if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
        if (vertical) {
          overlaySize = indeterminate ? kProgressVerticalIndeterminateOverlaySize
                                      : kProgressVerticalOverlaySize;
        } else {
          overlaySize = kProgressHorizontalVistaOverlaySize;
        }
      } else {
        overlaySize = kProgressHorizontalXPOverlaySize;
      }

      const double pixelsPerMillisecond = indeterminate
                                            ? kProgressIndeterminateSpeed
                                            : kProgressDeterminedVistaSpeed;
      const PRInt32 delay = indeterminate ? kProgressIndeterminateDelay
                                          : kProgressDeterminedVistaDelay;

      const PRInt32 frameSize = vertical ? widgetRect.bottom - widgetRect.top
                                         : widgetRect.right - widgetRect.left;
      const PRInt32 animationSize = frameSize + overlaySize +
                                     static_cast<PRInt32>(pixelsPerMillisecond * delay);
      const double interval = animationSize / pixelsPerMillisecond;
      
      double tempValue;
      double ratio = modf(PR_IntervalToMilliseconds(PR_IntervalNow())/interval,
                          &tempValue);
      
      
      if (!vertical && IsFrameRTL(aFrame)) {
        ratio = 1.0 - ratio;
      }
      PRInt32 dx = static_cast<PRInt32>(animationSize * ratio) - overlaySize;

      RECT overlayRect = widgetRect;
      if (vertical) {
        overlayRect.bottom -= dx;
        overlayRect.top = overlayRect.bottom - overlaySize;
      } else {
        overlayRect.left += dx;
        overlayRect.right = overlayRect.left + overlaySize;
      }

      PRInt32 overlayPart;
      if (vertical) {
        if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
          overlayPart = indeterminate ? PP_MOVEOVERLAY : PP_MOVEOVERLAYVERT;
        } else {
          overlayPart = PP_CHUNKVERT;
        }
      } else {
        overlayPart = WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION ?
          PP_MOVEOVERLAY : PP_CHUNK;
      }

      DrawThemeBackground(theme, hdc, overlayPart, state, &overlayRect,
                          &clipRect);
    }
  }


  nativeDrawing.EndNativeDrawing();

  if (nativeDrawing.ShouldRenderAgain())
    goto RENDER_AGAIN;

  nativeDrawing.PaintToContext();

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::GetWidgetBorder(nsDeviceContext* aContext, 
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntMargin* aResult)
{
  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return ClassicGetWidgetBorder(aContext, aFrame, aWidgetType, aResult); 

  (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right = 0;

  if (!WidgetIsContainer(aWidgetType) ||
      aWidgetType == NS_THEME_TOOLBOX || 
      aWidgetType == NS_THEME_WIN_MEDIA_TOOLBOX ||
      aWidgetType == NS_THEME_WIN_COMMUNICATIONS_TOOLBOX ||
      aWidgetType == NS_THEME_WIN_BROWSER_TAB_BAR_TOOLBOX ||
      aWidgetType == NS_THEME_STATUSBAR || 
      aWidgetType == NS_THEME_RESIZER || aWidgetType == NS_THEME_TAB_PANEL ||
      aWidgetType == NS_THEME_SCROLLBAR_TRACK_HORIZONTAL ||
      aWidgetType == NS_THEME_SCROLLBAR_TRACK_VERTICAL ||
      aWidgetType == NS_THEME_MENUITEM || aWidgetType == NS_THEME_CHECKMENUITEM ||
      aWidgetType == NS_THEME_RADIOMENUITEM || aWidgetType == NS_THEME_MENUPOPUP ||
      aWidgetType == NS_THEME_MENUIMAGE || aWidgetType == NS_THEME_MENUITEMTEXT ||
      aWidgetType == NS_THEME_TOOLBAR_SEPARATOR ||
      aWidgetType == NS_THEME_WINDOW_TITLEBAR ||
      aWidgetType == NS_THEME_WINDOW_TITLEBAR_MAXIMIZED ||
      aWidgetType == NS_THEME_WIN_GLASS || aWidgetType == NS_THEME_WIN_BORDERLESS_GLASS)
    return NS_OK; 

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  if (aWidgetType == NS_THEME_TOOLBAR) {
    
    if (state == 0)
      aResult->top = TB_SEPARATOR_HEIGHT;
    return NS_OK;
  }

  
  RECT outerRect; 
  outerRect.top = outerRect.left = 100;
  outerRect.right = outerRect.bottom = 200;
  RECT contentRect(outerRect);
  HRESULT res = GetThemeBackgroundContentRect(theme, NULL, part, state, &outerRect, &contentRect);
  
  if (FAILED(res))
    return NS_ERROR_FAILURE;

  
  
  aResult->top = contentRect.top - outerRect.top;
  aResult->bottom = outerRect.bottom - contentRect.bottom;
  aResult->left = contentRect.left - outerRect.left;
  aResult->right = outerRect.right - contentRect.right;

  
  if (aWidgetType == NS_THEME_TAB) {
    if (IsLeftToSelectedTab(aFrame))
      
      aResult->right = 0;
    else if (IsRightToSelectedTab(aFrame))
      
      aResult->left = 0;
  }

  if (aFrame && (aWidgetType == NS_THEME_TEXTFIELD || aWidgetType == NS_THEME_TEXTFIELD_MULTILINE)) {
    nsIContent* content = aFrame->GetContent();
    if (content && content->IsHTML()) {
      
      
      aResult->top++;
      aResult->left++;
      aResult->bottom++;
      aResult->right++;
    }
  }

  return NS_OK;
}

bool
nsNativeThemeWin::GetWidgetPadding(nsDeviceContext* aContext, 
                                   nsIFrame* aFrame,
                                   PRUint8 aWidgetType,
                                   nsIntMargin* aResult)
{
  switch (aWidgetType) {
    
    
    
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO:
      aResult->SizeTo(0, 0, 0, 0);
      return true;
  }

  HANDLE theme = GetTheme(aWidgetType);

  if (aWidgetType == NS_THEME_WINDOW_BUTTON_BOX ||
      aWidgetType == NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED) {
    aResult->SizeTo(0, 0, 0, 0);

    
    if (nsUXThemeData::CheckForCompositor())
      return true;

    
    if (aWidgetType == NS_THEME_WINDOW_BUTTON_BOX) {
      aResult->top = GetSystemMetrics(SM_CXFRAME);
    }
    return true;
  }

  
  if (aWidgetType == NS_THEME_WINDOW_TITLEBAR ||
      aWidgetType == NS_THEME_WINDOW_TITLEBAR_MAXIMIZED) {
    aResult->SizeTo(0, 0, 0, 0);
    
    
    
    if (aWidgetType == NS_THEME_WINDOW_TITLEBAR_MAXIMIZED)
      aResult->top = GetSystemMetrics(SM_CXFRAME);
    return true;
  }

  if (!theme)
    return ClassicGetWidgetPadding(aContext, aFrame, aWidgetType, aResult);

  if (aWidgetType == NS_THEME_MENUPOPUP)
  {
    SIZE popupSize;
    GetThemePartSize(theme, NULL, MENU_POPUPBORDERS,  0, NULL, TS_TRUE, &popupSize);
    aResult->top = aResult->bottom = popupSize.cy;
    aResult->left = aResult->right = popupSize.cx;
    return true;
  }

  if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
    if (aWidgetType == NS_THEME_TEXTFIELD ||
        aWidgetType == NS_THEME_TEXTFIELD_MULTILINE ||
        aWidgetType == NS_THEME_DROPDOWN)
    {
      
      if (aFrame->PresContext()->HasAuthorSpecifiedRules(aFrame, NS_AUTHOR_SPECIFIED_PADDING))
        return false;
    }

    





    if (aWidgetType == NS_THEME_TEXTFIELD || aWidgetType == NS_THEME_TEXTFIELD_MULTILINE) {
      aResult->top = aResult->bottom = 2;
      aResult->left = aResult->right = 2;
      return true;
    } else if (IsHTMLContent(aFrame) && aWidgetType == NS_THEME_DROPDOWN) {
      




      aResult->top = aResult->bottom = 1;
      aResult->left = aResult->right = 1;
      return true;
    }
  }

  PRInt32 right, left, top, bottom;
  right = left = top = bottom = 0;
  switch (aWidgetType)
  {
    case NS_THEME_MENUIMAGE:
        right = 8;
        left = 3;
        break;
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
        right = 8;
        left = 0;
        break;
    case NS_THEME_MENUITEMTEXT:
        
        
        {
          SIZE size(GetGutterSize(theme, NULL));
          left = size.cx + 2;
        }
        break;
    case NS_THEME_MENUSEPARATOR:
        {
          SIZE size(GetGutterSize(theme, NULL));
          left = size.cx + 5;
          top = 10;
          bottom = 7;
        }
        break;
    default:
        return false;
  }

  if (IsFrameRTL(aFrame))
  {
    aResult->right = left;
    aResult->left = right;
  }
  else
  {
    aResult->right = right;
    aResult->left = left;
  }
  
  return true;
}

bool
nsNativeThemeWin::GetWidgetOverflow(nsDeviceContext* aContext, 
                                    nsIFrame* aFrame,
                                    PRUint8 aOverflowRect,
                                    nsRect* aResult)
{
  






#if 0
  if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
    



    if (aWidgetType == NS_THEME_DROPDOWN_BUTTON &&
        IsHTMLContent(aFrame) &&
        !IsWidgetStyled(aFrame->GetParent()->PresContext(),
                        aFrame->GetParent(),
                        NS_THEME_DROPDOWN))
    {
      PRInt32 p2a = aContext->AppUnitsPerDevPixel();
      
      nsMargin m(0, p2a, p2a, p2a);
      aOverflowRect->Inflate (m);
      return true;
    }
  }
#endif

  return false;
}

NS_IMETHODIMP
nsNativeThemeWin::GetMinimumWidgetSize(nsRenderingContext* aContext, nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       nsIntSize* aResult, bool* aIsOverridable)
{
  (*aResult).width = (*aResult).height = 0;
  *aIsOverridable = true;

  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return ClassicGetMinimumWidgetSize(aContext, aFrame, aWidgetType, aResult, aIsOverridable);

  switch (aWidgetType) {
    case NS_THEME_GROUPBOX:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TOOLBOX:
    case NS_THEME_WIN_MEDIA_TOOLBOX:
    case NS_THEME_WIN_COMMUNICATIONS_TOOLBOX:
    case NS_THEME_WIN_BROWSER_TAB_BAR_TOOLBOX:
    case NS_THEME_TOOLBAR:
    case NS_THEME_STATUSBAR:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    case NS_THEME_TAB_PANELS:
    case NS_THEME_TAB_PANEL:
    case NS_THEME_LISTBOX:
    case NS_THEME_TREEVIEW:
    case NS_THEME_MENUITEMTEXT:
    case NS_THEME_WIN_GLASS:
    case NS_THEME_WIN_BORDERLESS_GLASS:
      return NS_OK; 
  }

  if (aWidgetType == NS_THEME_MENUITEM && IsTopLevelMenu(aFrame))
      return NS_OK; 

  
  
  
  THEMESIZE sizeReq = TS_TRUE; 
  switch (aWidgetType) {
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_DROPDOWN_BUTTON:
      return ClassicGetMinimumWidgetSize(aContext, aFrame, aWidgetType, aResult, aIsOverridable);

    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM:
      if(!IsTopLevelMenu(aFrame))
      {
        SIZE gutterSize(GetGutterSize(theme, NULL));
        aResult->width = gutterSize.cx;
        aResult->height = gutterSize.cy;
        return NS_OK;
      }
      break;

    case NS_THEME_MENUIMAGE:
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
      {
        SIZE boxSize(GetGutterSize(theme, NULL));
        aResult->width = boxSize.cx+2;
        aResult->height = boxSize.cy;
        *aIsOverridable = false;
      }

    case NS_THEME_MENUITEMTEXT:
      return NS_OK;

    case NS_THEME_MENUARROW:
      aResult->width = 26;
      aResult->height = 16;
      return NS_OK;

    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
      
      
      
      sizeReq = TS_MIN; 
      break;

    case NS_THEME_RESIZER:
      *aIsOverridable = false;
      break;

    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL:
    {
      *aIsOverridable = false;
      
      
      if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION) {
        if (aWidgetType == NS_THEME_SCALE_THUMB_HORIZONTAL) {
          aResult->width = 12;
          aResult->height = 20;
        }
        else {
          aResult->width = 20;
          aResult->height = 12;
        }
        return NS_OK;
      }
      break;
    }

    case NS_THEME_TOOLBAR_SEPARATOR:
      
      
      aResult->width = 6;
      return NS_OK;

    case NS_THEME_BUTTON:
      
      
      
      
      if (aFrame->GetContent()->IsHTML()) {
        sizeReq = TS_MIN;
      }
      break;

    case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
    case NS_THEME_WINDOW_BUTTON_RESTORE:
      
      
      
      QueryForButtonData(aFrame);
      aResult->width = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_RESTORE].cx;
      aResult->height = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_RESTORE].cy;
      
      if (WinUtils::GetWindowsVersion() == WinUtils::WINXP_VERSION) {
        aResult->width -= 4;
        aResult->height -= 4;
      }
      AddPaddingRect(aResult, CAPTIONBUTTON_RESTORE);
      *aIsOverridable = false;
      return NS_OK;

    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
      QueryForButtonData(aFrame);
      aResult->width = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_MINIMIZE].cx;
      aResult->height = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_MINIMIZE].cy;
      if (WinUtils::GetWindowsVersion() == WinUtils::WINXP_VERSION) {
        aResult->width -= 4;
        aResult->height -= 4;
      }
      AddPaddingRect(aResult, CAPTIONBUTTON_MINIMIZE);
      *aIsOverridable = false;
      return NS_OK;

    case NS_THEME_WINDOW_BUTTON_CLOSE:
      QueryForButtonData(aFrame);
      aResult->width = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_CLOSE].cx;
      aResult->height = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_CLOSE].cy;
      if (WinUtils::GetWindowsVersion() == WinUtils::WINXP_VERSION) {
        aResult->width -= 4;
        aResult->height -= 4;
      }
      AddPaddingRect(aResult, CAPTIONBUTTON_CLOSE);
      *aIsOverridable = false;
      return NS_OK;

    case NS_THEME_WINDOW_TITLEBAR:
    case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
      aResult->height = GetSystemMetrics(SM_CYCAPTION);
      aResult->height += GetSystemMetrics(SM_CYFRAME);
      *aIsOverridable = false;
      return NS_OK;

    case NS_THEME_WINDOW_BUTTON_BOX:
    case NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED:
      if (nsUXThemeData::CheckForCompositor()) {
        QueryForButtonData(aFrame);
        aResult->width = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_BUTTONBOX].cx;
        aResult->height = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_BUTTONBOX].cy
                          - GetSystemMetrics(SM_CYFRAME);
        if (aWidgetType == NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED) {
          aResult->width += 1;
          aResult->height -= 2;
        }
        *aIsOverridable = false;
        return NS_OK;
      }
      break;

    case NS_THEME_WINDOW_FRAME_LEFT:
    case NS_THEME_WINDOW_FRAME_RIGHT:
    case NS_THEME_WINDOW_FRAME_BOTTOM:
      aResult->width = GetSystemMetrics(SM_CXFRAME);
      aResult->height = GetSystemMetrics(SM_CYFRAME);
      *aIsOverridable = false;
      return NS_OK;
  }

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  HDC hdc = gfxWindowsPlatform::GetPlatform()->GetScreenDC();
  if (!hdc)
    return NS_ERROR_FAILURE;

  SIZE sz;
  GetThemePartSize(theme, hdc, part, state, NULL, sizeReq, &sz);
  aResult->width = sz.cx;
  aResult->height = sz.cy;

  switch(aWidgetType) {
    case NS_THEME_SPINNER_UP_BUTTON:
    case NS_THEME_SPINNER_DOWN_BUTTON:
      aResult->width++;
      aResult->height = aResult->height / 2 + 1;
      break;

    case NS_THEME_MENUSEPARATOR:
    {
      SIZE gutterSize(GetGutterSize(theme,hdc));
      aResult->width += gutterSize.cx;
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                     nsIAtom* aAttribute, bool* aShouldRepaint)
{
  
  if (aWidgetType == NS_THEME_TOOLBOX ||
      aWidgetType == NS_THEME_WIN_MEDIA_TOOLBOX ||
      aWidgetType == NS_THEME_WIN_COMMUNICATIONS_TOOLBOX ||
      aWidgetType == NS_THEME_WIN_BROWSER_TAB_BAR_TOOLBOX ||
      aWidgetType == NS_THEME_TOOLBAR ||
      aWidgetType == NS_THEME_STATUSBAR || aWidgetType == NS_THEME_STATUSBAR_PANEL ||
      aWidgetType == NS_THEME_STATUSBAR_RESIZER_PANEL ||
      aWidgetType == NS_THEME_PROGRESSBAR_CHUNK ||
      aWidgetType == NS_THEME_PROGRESSBAR_CHUNK_VERTICAL ||
      aWidgetType == NS_THEME_PROGRESSBAR ||
      aWidgetType == NS_THEME_PROGRESSBAR_VERTICAL ||
      aWidgetType == NS_THEME_TOOLTIP ||
      aWidgetType == NS_THEME_TAB_PANELS ||
      aWidgetType == NS_THEME_TAB_PANEL ||
      aWidgetType == NS_THEME_TOOLBAR_SEPARATOR ||
      aWidgetType == NS_THEME_WIN_GLASS ||
      aWidgetType == NS_THEME_WIN_BORDERLESS_GLASS) {
    *aShouldRepaint = false;
    return NS_OK;
  }

  if (aWidgetType == NS_THEME_WINDOW_TITLEBAR ||
      aWidgetType == NS_THEME_WINDOW_TITLEBAR_MAXIMIZED ||
      aWidgetType == NS_THEME_WINDOW_FRAME_LEFT ||
      aWidgetType == NS_THEME_WINDOW_FRAME_RIGHT ||
      aWidgetType == NS_THEME_WINDOW_FRAME_BOTTOM ||
      aWidgetType == NS_THEME_WINDOW_BUTTON_CLOSE ||
      aWidgetType == NS_THEME_WINDOW_BUTTON_MINIMIZE ||
      aWidgetType == NS_THEME_WINDOW_BUTTON_MINIMIZE ||
      aWidgetType == NS_THEME_WINDOW_BUTTON_RESTORE) {
    *aShouldRepaint = true;
    return NS_OK;
  }

  
  if (WinUtils::GetWindowsVersion() < WinUtils::VISTA_VERSION &&
      (aWidgetType == NS_THEME_SCROLLBAR_TRACK_VERTICAL || 
      aWidgetType == NS_THEME_SCROLLBAR_TRACK_HORIZONTAL)) {
    *aShouldRepaint = false;
    return NS_OK;
  }

  
  
  if (WinUtils::GetWindowsVersion() >= WinUtils::VISTA_VERSION &&
      (aWidgetType == NS_THEME_DROPDOWN || aWidgetType == NS_THEME_DROPDOWN_BUTTON) &&
      IsHTMLContent(aFrame))
  {
    *aShouldRepaint = true;
    return NS_OK;
  }

  
  
  
  if (!aAttribute) {
    
    *aShouldRepaint = true;
  }
  else {
    
    
    *aShouldRepaint = false;
    if (aAttribute == nsGkAtoms::disabled ||
        aAttribute == nsGkAtoms::checked ||
        aAttribute == nsGkAtoms::selected ||
        aAttribute == nsGkAtoms::readonly ||
        aAttribute == nsGkAtoms::open ||
        aAttribute == nsGkAtoms::menuactive ||
        aAttribute == nsGkAtoms::focused)
      *aShouldRepaint = true;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::ThemeChanged()
{
  nsUXThemeData::Invalidate();
  return NS_OK;
}

bool 
nsNativeThemeWin::ThemeSupportsWidget(nsPresContext* aPresContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType)
{
  
  

  if (aPresContext && !aPresContext->PresShell()->IsThemeSupportEnabled())
    return false;

  HANDLE theme = NULL;
  if (aWidgetType == NS_THEME_CHECKBOX_CONTAINER)
    theme = GetTheme(NS_THEME_CHECKBOX);
  else if (aWidgetType == NS_THEME_RADIO_CONTAINER)
    theme = GetTheme(NS_THEME_RADIO);
  else
    theme = GetTheme(aWidgetType);

  if ((theme) || (!theme && ClassicThemeSupportsWidget(aPresContext, aFrame, aWidgetType)))
    
    return (!IsWidgetStyled(aPresContext, aFrame, aWidgetType));
  
  return false;
}

bool 
nsNativeThemeWin::WidgetIsContainer(PRUint8 aWidgetType)
{
  
  if (aWidgetType == NS_THEME_DROPDOWN_BUTTON || 
      aWidgetType == NS_THEME_RADIO ||
      aWidgetType == NS_THEME_CHECKBOX)
    return false;
  return true;
}

bool
nsNativeThemeWin::ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType)
{
  return false;
}

bool
nsNativeThemeWin::ThemeNeedsComboboxDropmarker()
{
  return true;
}

nsITheme::Transparency
nsNativeThemeWin::GetWidgetTransparency(nsIFrame* aFrame, PRUint8 aWidgetType)
{
  switch (aWidgetType) {
  case NS_THEME_SCROLLBAR_SMALL:
  case NS_THEME_SCROLLBAR:
  case NS_THEME_STATUSBAR:
    
    
    
    
    return eOpaque;
  case NS_THEME_WIN_GLASS:
  case NS_THEME_WIN_BORDERLESS_GLASS:
  case NS_THEME_SCALE_HORIZONTAL:
  case NS_THEME_SCALE_VERTICAL:
    return eTransparent;
  }

  HANDLE theme = GetTheme(aWidgetType);
  
  if (!theme) {
    
    if (aWidgetType == NS_THEME_MENUPOPUP || aWidgetType == NS_THEME_TOOLTIP) {
      return eOpaque;
    }
    return eUnknownTransparency;
  }

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  
  NS_ENSURE_SUCCESS(rv, eUnknownTransparency);

  if (part <= 0) {
    
    
    return eUnknownTransparency;
  }

  if (IsThemeBackgroundPartiallyTransparent(theme, part, state))
    return eTransparent;
  return eOpaque;
}



bool 
nsNativeThemeWin::ClassicThemeSupportsWidget(nsPresContext* aPresContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType)
{
  switch (aWidgetType) {
    case NS_THEME_RESIZER:
    {
      
      
      
      nsIFrame* parentFrame = aFrame->GetParent();
      return (!parentFrame || parentFrame->GetType() != nsGkAtoms::scrollFrame);
    }
    case NS_THEME_MENUBAR:
    case NS_THEME_MENUPOPUP:
      
      if (!nsUXThemeData::sFlatMenus)
        return false;
    case NS_THEME_BUTTON:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO:
    case NS_THEME_GROUPBOX:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCALE_HORIZONTAL:
    case NS_THEME_SCALE_VERTICAL:
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL:
    case NS_THEME_DROPDOWN_BUTTON:
    case NS_THEME_SPINNER_UP_BUTTON:
    case NS_THEME_SPINNER_DOWN_BUTTON:
    case NS_THEME_LISTBOX:
    case NS_THEME_TREEVIEW:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_DROPDOWN:
    case NS_THEME_TOOLTIP:
    case NS_THEME_STATUSBAR:
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    case NS_THEME_TAB:
    case NS_THEME_TAB_PANEL:
    case NS_THEME_TAB_PANELS:
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM:
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
    case NS_THEME_MENUARROW:
    case NS_THEME_MENUSEPARATOR:
    case NS_THEME_MENUITEMTEXT:
    case NS_THEME_WINDOW_TITLEBAR:
    case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
    case NS_THEME_WINDOW_FRAME_LEFT:
    case NS_THEME_WINDOW_FRAME_RIGHT:
    case NS_THEME_WINDOW_FRAME_BOTTOM:
    case NS_THEME_WINDOW_BUTTON_CLOSE:
    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
    case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
    case NS_THEME_WINDOW_BUTTON_RESTORE:
    case NS_THEME_WINDOW_BUTTON_BOX:
    case NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED:
      return true;
  }
  return false;
}

nsresult
nsNativeThemeWin::ClassicGetWidgetBorder(nsDeviceContext* aContext, 
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsIntMargin* aResult)
{
  switch (aWidgetType) {
    case NS_THEME_GROUPBOX:
    case NS_THEME_BUTTON:
      (*aResult).top = (*aResult).left = (*aResult).bottom = (*aResult).right = 2; 
      break;
    case NS_THEME_STATUSBAR:
      (*aResult).bottom = (*aResult).left = (*aResult).right = 0;
      (*aResult).top = 2;
      break;
    case NS_THEME_LISTBOX:
    case NS_THEME_TREEVIEW:
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_TAB:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
      (*aResult).top = (*aResult).left = (*aResult).bottom = (*aResult).right = 2;
      break;
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL: {
      (*aResult).top = 1;      
      (*aResult).left = 1;
      (*aResult).bottom = 1;
      (*aResult).right = aFrame->GetNextSibling() ? 3 : 1;
      break;
    }    
    case NS_THEME_TOOLTIP:
      (*aResult).top = (*aResult).left = (*aResult).bottom = (*aResult).right = 1;
      break;
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
      (*aResult).top = (*aResult).left = (*aResult).bottom = (*aResult).right = 1;
      break;
    case NS_THEME_MENUBAR:
      (*aResult).top = (*aResult).left = (*aResult).bottom = (*aResult).right = 0;
      break;
    case NS_THEME_MENUPOPUP:
      (*aResult).top = (*aResult).left = (*aResult).bottom = (*aResult).right = 3;
      break;
    default:
      (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right = 0;
      break;
  }
  return NS_OK;
}

bool
nsNativeThemeWin::ClassicGetWidgetPadding(nsDeviceContext* aContext,
                                   nsIFrame* aFrame,
                                   PRUint8 aWidgetType,
                                   nsIntMargin* aResult)
{
  switch (aWidgetType) {
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM: {
      PRInt32 part, state;
      bool focused;

      if (NS_FAILED(ClassicGetThemePartAndState(aFrame, aWidgetType, part, state, focused)))
        return false;

      if (part == 1) { 
        if (nsUXThemeData::sFlatMenus || !(state & DFCS_PUSHED)) {
          (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right = 2;
        }
        else {
          
          (*aResult).top = (*aResult).left = 3;
          (*aResult).bottom = (*aResult).right = 1;
        }
      }
      else {
        (*aResult).top = 0;
        (*aResult).bottom = (*aResult).left = (*aResult).right = 2;
      }
      return true;
    }
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
      (*aResult).top = (*aResult).left = (*aResult).bottom = (*aResult).right = 1;
      return true;
    default:
      return false;
  }
}

nsresult
nsNativeThemeWin::ClassicGetMinimumWidgetSize(nsRenderingContext* aContext, nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       nsIntSize* aResult, bool* aIsOverridable)
{
  (*aResult).width = (*aResult).height = 0;
  *aIsOverridable = true;
  switch (aWidgetType) {
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX:
      (*aResult).width = (*aResult).height = 13;
      break;
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
    case NS_THEME_MENUARROW:
      (*aResult).width = ::GetSystemMetrics(SM_CXMENUCHECK);
      (*aResult).height = ::GetSystemMetrics(SM_CYMENUCHECK);
      break;
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
      (*aResult).width = ::GetSystemMetrics(SM_CXVSCROLL);
      (*aResult).height = ::GetSystemMetrics(SM_CYVSCROLL);
      *aIsOverridable = false;
      break;
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
      (*aResult).width = ::GetSystemMetrics(SM_CXHSCROLL);
      (*aResult).height = ::GetSystemMetrics(SM_CYHSCROLL);
      *aIsOverridable = false;
      break;
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
      
      
      

        
      break;
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
      (*aResult).width = 12;
      (*aResult).height = 20;
      *aIsOverridable = false;
      break;
    case NS_THEME_SCALE_THUMB_VERTICAL:
      (*aResult).width = 20;
      (*aResult).height = 12;
      *aIsOverridable = false;
      break;
    case NS_THEME_DROPDOWN_BUTTON:
      (*aResult).width = ::GetSystemMetrics(SM_CXVSCROLL);
      break;
    case NS_THEME_DROPDOWN:
    case NS_THEME_BUTTON:
    case NS_THEME_GROUPBOX:
    case NS_THEME_LISTBOX:
    case NS_THEME_TREEVIEW:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
    case NS_THEME_DROPDOWN_TEXTFIELD:      
    case NS_THEME_STATUSBAR:
    case NS_THEME_STATUSBAR_PANEL:      
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    case NS_THEME_TOOLTIP:
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_TAB:
    case NS_THEME_TAB_PANEL:
    case NS_THEME_TAB_PANELS:
      
      break;
    case NS_THEME_RESIZER: {     
      NONCLIENTMETRICS nc;
      nc.cbSize = sizeof(nc);
      if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(nc), &nc, 0))
        (*aResult).width = (*aResult).height = abs(nc.lfStatusFont.lfHeight) + 4;
      else
        (*aResult).width = (*aResult).height = 15;
      *aIsOverridable = false;
      break;
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
      (*aResult).width = ::GetSystemMetrics(SM_CXVSCROLL);
      (*aResult).height = ::GetSystemMetrics(SM_CYVTHUMB);
      
      
      if (!GetTheme(aWidgetType))
        (*aResult).height >>= 1;
      *aIsOverridable = false;
      break;
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
      (*aResult).width = ::GetSystemMetrics(SM_CXHTHUMB);
      (*aResult).height = ::GetSystemMetrics(SM_CYHSCROLL);
      
      
      if (!GetTheme(aWidgetType))
        (*aResult).width >>= 1;
      *aIsOverridable = false;
      break;
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
      (*aResult).width = ::GetSystemMetrics(SM_CXHTHUMB) << 1;
      break;
    }
    case NS_THEME_MENUSEPARATOR:
    {
      aResult->width = 0;
      aResult->height = 10;
      break;
    }

    case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
    case NS_THEME_WINDOW_TITLEBAR:
      aResult->height = GetSystemMetrics(SM_CYCAPTION);
      aResult->height += GetSystemMetrics(SM_CYFRAME);
      aResult->width = 0;
    break;
    case NS_THEME_WINDOW_FRAME_LEFT:
    case NS_THEME_WINDOW_FRAME_RIGHT:
      aResult->width = GetSystemMetrics(SM_CXFRAME);
      aResult->height = 0;
    break;

    case NS_THEME_WINDOW_FRAME_BOTTOM:
      aResult->height = GetSystemMetrics(SM_CYFRAME);
      aResult->width = 0;
    break;

    case NS_THEME_WINDOW_BUTTON_CLOSE:
    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
    case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
    case NS_THEME_WINDOW_BUTTON_RESTORE:
      aResult->width = GetSystemMetrics(SM_CXSIZE);
      aResult->height = GetSystemMetrics(SM_CYSIZE);
      
      
      aResult->width -= 2;
      aResult->height -= 4;
      if (aWidgetType == NS_THEME_WINDOW_BUTTON_MINIMIZE) {
        AddPaddingRect(aResult, CAPTIONBUTTON_MINIMIZE);
      }
      else if (aWidgetType == NS_THEME_WINDOW_BUTTON_MAXIMIZE ||
               aWidgetType == NS_THEME_WINDOW_BUTTON_RESTORE) {
        AddPaddingRect(aResult, CAPTIONBUTTON_RESTORE);
      }
      else if (aWidgetType == NS_THEME_WINDOW_BUTTON_CLOSE) {
        AddPaddingRect(aResult, CAPTIONBUTTON_CLOSE);
      }
    break;

    default:
      return NS_ERROR_FAILURE;
  }  
  return NS_OK;
}


nsresult nsNativeThemeWin::ClassicGetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType,
                                 PRInt32& aPart, PRInt32& aState, bool& aFocused)
{  
  aFocused = false;
  switch (aWidgetType) {
    case NS_THEME_BUTTON: {
      nsEventStates contentState;

      aPart = DFC_BUTTON;
      aState = DFCS_BUTTONPUSH;
      aFocused = false;

      contentState = GetContentState(aFrame, aWidgetType);
      if (IsDisabled(aFrame, contentState))
        aState |= DFCS_INACTIVE;
      else if (IsOpenButton(aFrame))
        aState |= DFCS_PUSHED;
      else if (IsCheckedButton(aFrame))
        aState |= DFCS_CHECKED;
      else {
        if (contentState.HasAllStates(NS_EVENT_STATE_ACTIVE | NS_EVENT_STATE_HOVER)) {
          aState |= DFCS_PUSHED;
          const nsStyleUserInterface *uiData = aFrame->GetStyleUserInterface();
          
          if (uiData->mUserFocus == NS_STYLE_USER_FOCUS_NORMAL) {
            if (!aFrame->GetContent()->IsHTML())
              aState |= DFCS_FLAT;

            aFocused = true;
          }
        }
        if (contentState.HasState(NS_EVENT_STATE_FOCUS) ||
            (aState == DFCS_BUTTONPUSH && IsDefaultButton(aFrame))) {
          aFocused = true;
        }

      }

      return NS_OK;
    }
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO: {
      nsEventStates contentState;
      aFocused = false;

      aPart = DFC_BUTTON;
      aState = 0;
      nsIContent* content = aFrame->GetContent();
      bool isCheckbox = (aWidgetType == NS_THEME_CHECKBOX);
      bool isChecked = GetCheckedOrSelected(aFrame, !isCheckbox);
      bool isIndeterminate = isCheckbox && GetIndeterminate(aFrame);

      if (isCheckbox) {
        
        if (isIndeterminate) {
          aState = DFCS_BUTTON3STATE | DFCS_CHECKED;
        } else {
          aState = DFCS_BUTTONCHECK;
        }
      } else {
        aState = DFCS_BUTTONRADIO;
      }
      if (isChecked) {
        aState |= DFCS_CHECKED;
      }

      contentState = GetContentState(aFrame, aWidgetType);
      if (!content->IsXUL() &&
          contentState.HasState(NS_EVENT_STATE_FOCUS)) {
        aFocused = true;
      }

      if (IsDisabled(aFrame, contentState)) {
        aState |= DFCS_INACTIVE;
      } else if (contentState.HasAllStates(NS_EVENT_STATE_ACTIVE |
                                           NS_EVENT_STATE_HOVER)) {
        aState |= DFCS_PUSHED;
      }

      return NS_OK;
    }
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM: {
      bool isTopLevel = false;
      bool isOpen = false;
      bool isContainer = false;
      nsMenuFrame *menuFrame = do_QueryFrame(aFrame);
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      
      
      
      aPart = 0;
      aState = 0;

      if (menuFrame) {
        
        
        
        isTopLevel = menuFrame->IsOnMenuBar();
        isOpen = menuFrame->IsOpen();
        isContainer = menuFrame->IsMenu();
      }

      if (IsDisabled(aFrame, eventState))
        aState |= DFCS_INACTIVE;

      if (isTopLevel) {
        aPart = 1;
        if (isOpen)
          aState |= DFCS_PUSHED;
      }

      if (IsMenuActive(aFrame, aWidgetType))
        aState |= DFCS_HOT;

      return NS_OK;
    }
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
    case NS_THEME_MENUARROW: {
      aState = 0;
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      if (IsDisabled(aFrame, eventState))
        aState |= DFCS_INACTIVE;
      if (IsMenuActive(aFrame, aWidgetType))
        aState |= DFCS_HOT;

      if (aWidgetType == NS_THEME_MENUCHECKBOX || aWidgetType == NS_THEME_MENURADIO) {
        if (IsCheckedButton(aFrame))
          aState |= DFCS_CHECKED;
      } else if (IsFrameRTL(aFrame)) {
          aState |= DFCS_RTL;
      }
      return NS_OK;
    }
    case NS_THEME_LISTBOX:
    case NS_THEME_TREEVIEW:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:     
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:      
    case NS_THEME_SCALE_HORIZONTAL:
    case NS_THEME_SCALE_VERTICAL:
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL:
    case NS_THEME_STATUSBAR:
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    case NS_THEME_TOOLTIP:
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_TAB:
    case NS_THEME_TAB_PANEL:
    case NS_THEME_TAB_PANELS:
    case NS_THEME_MENUBAR:
    case NS_THEME_MENUPOPUP:
    case NS_THEME_GROUPBOX:
      
      return NS_OK;
    case NS_THEME_DROPDOWN_BUTTON: {

      aPart = DFC_SCROLL;
      aState = DFCS_SCROLLCOMBOBOX;

      nsIFrame* parentFrame = aFrame->GetParent();
      bool isHTML = IsHTMLContent(aFrame);
      bool isMenulist = !isHTML && parentFrame->GetType() == nsGkAtoms::menuFrame;
      bool isOpen = false;

      
      if (isHTML || isMenulist)
        aFrame = parentFrame;

      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      if (IsDisabled(aFrame, eventState)) {
        aState |= DFCS_INACTIVE;
        return NS_OK;
      }

      if (isHTML) {
        nsIComboboxControlFrame* ccf = do_QueryFrame(aFrame);
        isOpen = (ccf && ccf->IsDroppedDown());
      }
      else
        isOpen = IsOpenButton(aFrame);

      
      
      if (isOpen && (isHTML || isMenulist))
        return NS_OK;

      
      if (eventState.HasState(NS_EVENT_STATE_ACTIVE))
        aState |= DFCS_PUSHED | DFCS_FLAT;

      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT: {
      nsEventStates contentState = GetContentState(aFrame, aWidgetType);

      aPart = DFC_SCROLL;
      switch (aWidgetType) {
        case NS_THEME_SCROLLBAR_BUTTON_UP:
          aState = DFCS_SCROLLUP;
          break;
        case NS_THEME_SCROLLBAR_BUTTON_DOWN:
          aState = DFCS_SCROLLDOWN;
          break;
        case NS_THEME_SCROLLBAR_BUTTON_LEFT:
          aState = DFCS_SCROLLLEFT;
          break;
        case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
          aState = DFCS_SCROLLRIGHT;
          break;
      }

      if (IsDisabled(aFrame, contentState))
        aState |= DFCS_INACTIVE;
      else {
        if (contentState.HasAllStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE))
          aState |= DFCS_PUSHED | DFCS_FLAT;
      }

      return NS_OK;
    }
    case NS_THEME_SPINNER_UP_BUTTON:
    case NS_THEME_SPINNER_DOWN_BUTTON: {
      nsEventStates contentState = GetContentState(aFrame, aWidgetType);

      aPart = DFC_SCROLL;
      switch (aWidgetType) {
        case NS_THEME_SPINNER_UP_BUTTON:
          aState = DFCS_SCROLLUP;
          break;
        case NS_THEME_SPINNER_DOWN_BUTTON:
          aState = DFCS_SCROLLDOWN;
          break;
      }

      if (IsDisabled(aFrame, contentState))
        aState |= DFCS_INACTIVE;
      else {
        if (contentState.HasAllStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE))
          aState |= DFCS_PUSHED;
      }

      return NS_OK;    
    }
    case NS_THEME_RESIZER:    
      aPart = DFC_SCROLL;
      aState = (IsFrameRTL(aFrame)) ?
               DFCS_SCROLLSIZEGRIPRIGHT : DFCS_SCROLLSIZEGRIP;
      return NS_OK;
    case NS_THEME_MENUSEPARATOR:
      aPart = 0;
      aState = 0;
      return NS_OK;
    case NS_THEME_WINDOW_TITLEBAR:
      aPart = mozilla::widget::themeconst::WP_CAPTION;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
      aPart = mozilla::widget::themeconst::WP_MAXCAPTION;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_FRAME_LEFT:
      aPart = mozilla::widget::themeconst::WP_FRAMELEFT;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_FRAME_RIGHT:
      aPart = mozilla::widget::themeconst::WP_FRAMERIGHT;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_FRAME_BOTTOM:
      aPart = mozilla::widget::themeconst::WP_FRAMEBOTTOM;
      aState = GetTopLevelWindowActiveState(aFrame);
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_CLOSE:
      aPart = DFC_CAPTION;
      aState = DFCS_CAPTIONCLOSE |
               GetClassicWindowFrameButtonState(GetContentState(aFrame,
                                                                aWidgetType));
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
      aPart = DFC_CAPTION;
      aState = DFCS_CAPTIONMIN |
               GetClassicWindowFrameButtonState(GetContentState(aFrame,
                                                                aWidgetType));
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
      aPart = DFC_CAPTION;
      aState = DFCS_CAPTIONMAX |
               GetClassicWindowFrameButtonState(GetContentState(aFrame,
                                                                aWidgetType));
      return NS_OK;
    case NS_THEME_WINDOW_BUTTON_RESTORE:
      aPart = DFC_CAPTION;
      aState = DFCS_CAPTIONRESTORE |
               GetClassicWindowFrameButtonState(GetContentState(aFrame,
                                                                aWidgetType));
      return NS_OK;
  }
  return NS_ERROR_FAILURE;
}



static void DrawTab(HDC hdc, const RECT& R, PRInt32 aPosition, bool aSelected,
                    bool aDrawLeft, bool aDrawRight)
{
  PRInt32 leftFlag, topFlag, rightFlag, lightFlag, shadeFlag;  
  RECT topRect, sideRect, bottomRect, lightRect, shadeRect;
  PRInt32 selectedOffset, lOffset, rOffset;

  selectedOffset = aSelected ? 1 : 0;
  lOffset = aDrawLeft ? 2 : 0;
  rOffset = aDrawRight ? 2 : 0;

  
  switch (aPosition) {
    case BF_LEFT:
      leftFlag = BF_TOP; topFlag = BF_LEFT;
      rightFlag = BF_BOTTOM;
      lightFlag = BF_DIAGONAL_ENDTOPRIGHT;
      shadeFlag = BF_DIAGONAL_ENDBOTTOMRIGHT;

      ::SetRect(&topRect, R.left, R.top+lOffset, R.right, R.bottom-rOffset);
      ::SetRect(&sideRect, R.left+2, R.top, R.right-2+selectedOffset, R.bottom);
      ::SetRect(&bottomRect, R.right-2, R.top, R.right, R.bottom);
      ::SetRect(&lightRect, R.left, R.top, R.left+3, R.top+3);
      ::SetRect(&shadeRect, R.left+1, R.bottom-2, R.left+2, R.bottom-1);
      break;
    case BF_TOP:    
      leftFlag = BF_LEFT; topFlag = BF_TOP;
      rightFlag = BF_RIGHT;
      lightFlag = BF_DIAGONAL_ENDTOPRIGHT;
      shadeFlag = BF_DIAGONAL_ENDBOTTOMRIGHT;

      ::SetRect(&topRect, R.left+lOffset, R.top, R.right-rOffset, R.bottom);
      ::SetRect(&sideRect, R.left, R.top+2, R.right, R.bottom-1+selectedOffset);
      ::SetRect(&bottomRect, R.left, R.bottom-1, R.right, R.bottom);
      ::SetRect(&lightRect, R.left, R.top, R.left+3, R.top+3);      
      ::SetRect(&shadeRect, R.right-2, R.top+1, R.right-1, R.top+2);      
      break;
    case BF_RIGHT:    
      leftFlag = BF_TOP; topFlag = BF_RIGHT;
      rightFlag = BF_BOTTOM;
      lightFlag = BF_DIAGONAL_ENDTOPLEFT;
      shadeFlag = BF_DIAGONAL_ENDBOTTOMLEFT;

      ::SetRect(&topRect, R.left, R.top+lOffset, R.right, R.bottom-rOffset);
      ::SetRect(&sideRect, R.left+2-selectedOffset, R.top, R.right-2, R.bottom);
      ::SetRect(&bottomRect, R.left, R.top, R.left+2, R.bottom);
      ::SetRect(&lightRect, R.right-3, R.top, R.right-1, R.top+2);
      ::SetRect(&shadeRect, R.right-2, R.bottom-3, R.right, R.bottom-1);
      break;
    case BF_BOTTOM:    
      leftFlag = BF_LEFT; topFlag = BF_BOTTOM;
      rightFlag = BF_RIGHT;
      lightFlag = BF_DIAGONAL_ENDTOPLEFT;
      shadeFlag = BF_DIAGONAL_ENDBOTTOMLEFT;

      ::SetRect(&topRect, R.left+lOffset, R.top, R.right-rOffset, R.bottom);
      ::SetRect(&sideRect, R.left, R.top+2-selectedOffset, R.right, R.bottom-2);
      ::SetRect(&bottomRect, R.left, R.top, R.right, R.top+2);
      ::SetRect(&lightRect, R.left, R.bottom-3, R.left+2, R.bottom-1);
      ::SetRect(&shadeRect, R.right-2, R.bottom-3, R.right, R.bottom-1);
      break;
  }

  
  ::FillRect(hdc, &R, (HBRUSH) (COLOR_3DFACE+1) );

  
  ::DrawEdge(hdc, &topRect, EDGE_RAISED, BF_SOFT | topFlag);

  
  if (!aSelected)
    ::DrawEdge(hdc, &bottomRect, EDGE_RAISED, BF_SOFT | topFlag);

  
  if (!aDrawLeft)
    leftFlag = 0;
  if (!aDrawRight)
    rightFlag = 0;
  ::DrawEdge(hdc, &sideRect, EDGE_RAISED, BF_SOFT | leftFlag | rightFlag);

  
  if (aDrawLeft)
    ::DrawEdge(hdc, &lightRect, EDGE_RAISED, BF_SOFT | lightFlag);

  if (aDrawRight)
    ::DrawEdge(hdc, &shadeRect, EDGE_RAISED, BF_SOFT | shadeFlag);
}

static void DrawMenuImage(HDC hdc, const RECT& rc, PRInt32 aComponent, PRUint32 aColor)
{
  
  
  
  HDC hMemoryDC = ::CreateCompatibleDC(hdc);
  if (hMemoryDC) {
    
    
    int checkW = ::GetSystemMetrics(SM_CXMENUCHECK);
    int checkH = ::GetSystemMetrics(SM_CYMENUCHECK);

    HBITMAP hMonoBitmap = ::CreateBitmap(checkW, checkH, 1, 1, NULL);
    if (hMonoBitmap) {

      HBITMAP hPrevBitmap = (HBITMAP) ::SelectObject(hMemoryDC, hMonoBitmap);
      if (hPrevBitmap) {

        
        
        RECT imgRect = { 0, 0, checkW, checkH };
        POINT imgPos = {
              rc.left + (rc.right  - rc.left - checkW) / 2,
              rc.top  + (rc.bottom - rc.top  - checkH) / 2
            };

        
        if (aComponent == DFCS_MENUCHECK || aComponent == DFCS_MENUBULLET)
          imgPos.y++;

        ::DrawFrameControl(hMemoryDC, &imgRect, DFC_MENU, aComponent);
        COLORREF oldTextCol = ::SetTextColor(hdc, 0x00000000);
        COLORREF oldBackCol = ::SetBkColor(hdc, 0x00FFFFFF);
        ::BitBlt(hdc, imgPos.x, imgPos.y, checkW, checkH, hMemoryDC, 0, 0, SRCAND);
        ::SetTextColor(hdc, ::GetSysColor(aColor));
        ::SetBkColor(hdc, 0x00000000);
        ::BitBlt(hdc, imgPos.x, imgPos.y, checkW, checkH, hMemoryDC, 0, 0, SRCPAINT);
        ::SetTextColor(hdc, oldTextCol);
        ::SetBkColor(hdc, oldBackCol);
        ::SelectObject(hMemoryDC, hPrevBitmap);
      }
      ::DeleteObject(hMonoBitmap);
    }
    ::DeleteDC(hMemoryDC);
  }
}

void nsNativeThemeWin::DrawCheckedRect(HDC hdc, const RECT& rc, PRInt32 fore, PRInt32 back,
                                       HBRUSH defaultBack)
{
  static WORD patBits[8] = {
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55
  };
        
  HBITMAP patBmp = ::CreateBitmap(8, 8, 1, 1, patBits);
  if (patBmp) {
    HBRUSH brush = (HBRUSH) ::CreatePatternBrush(patBmp);
    if (brush) {        
      COLORREF oldForeColor = ::SetTextColor(hdc, ::GetSysColor(fore));
      COLORREF oldBackColor = ::SetBkColor(hdc, ::GetSysColor(back));
      POINT vpOrg;

      ::UnrealizeObject(brush);
      ::GetViewportOrgEx(hdc, &vpOrg);
      ::SetBrushOrgEx(hdc, vpOrg.x + rc.left, vpOrg.y + rc.top, NULL);
      HBRUSH oldBrush = (HBRUSH) ::SelectObject(hdc, brush);
      ::FillRect(hdc, &rc, brush);
      ::SetTextColor(hdc, oldForeColor);
      ::SetBkColor(hdc, oldBackColor);
      ::SelectObject(hdc, oldBrush);
      ::DeleteObject(brush);          
    }
    else
      ::FillRect(hdc, &rc, defaultBack);
  
    ::DeleteObject(patBmp);
  }
}

nsresult nsNativeThemeWin::ClassicDrawWidgetBackground(nsRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aDirtyRect)
{
  PRInt32 part, state;
  bool focused;
  nsresult rv;
  rv = ClassicGetThemePartAndState(aFrame, aWidgetType, part, state, focused);
  if (NS_FAILED(rv))
    return rv;

  gfxFloat p2a = gfxFloat(aContext->AppUnitsPerDevPixel());
  RECT widgetRect;
  gfxRect tr(aRect.x, aRect.y, aRect.width, aRect.height),
          dr(aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);

  tr.ScaleInverse(p2a);
  dr.ScaleInverse(p2a);

  nsRefPtr<gfxContext> ctx = aContext->ThebesContext();

  gfxWindowsNativeDrawing nativeDrawing(ctx, dr, GetWidgetNativeDrawingFlags(aWidgetType));

RENDER_AGAIN:

  HDC hdc = nativeDrawing.BeginNativeDrawing();
  if (!hdc)
    return NS_ERROR_FAILURE;

  nativeDrawing.TransformToNativeRect(tr, widgetRect);

  rv = NS_OK;
  switch (aWidgetType) { 
    
    case NS_THEME_BUTTON: {
      if (focused) {
        
        HBRUSH brush;        
        brush = ::GetSysColorBrush(COLOR_3DDKSHADOW);
        if (brush)
          ::FrameRect(hdc, &widgetRect, brush);
        InflateRect(&widgetRect, -1, -1);
      }
      
    }
    
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SPINNER_UP_BUTTON:
    case NS_THEME_SPINNER_DOWN_BUTTON:
    case NS_THEME_DROPDOWN_BUTTON:
    case NS_THEME_RESIZER: {
      PRInt32 oldTA;
      
      oldTA = ::SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
      ::DrawFrameControl(hdc, &widgetRect, part, state);
      ::SetTextAlign(hdc, oldTA);

      
      
      if (focused && (aWidgetType == NS_THEME_CHECKBOX || aWidgetType == NS_THEME_RADIO)) {
        
        POINT vpOrg;
        ::GetViewportOrgEx(hdc, &vpOrg);
        ::SetBrushOrgEx(hdc, vpOrg.x + widgetRect.left, vpOrg.y + widgetRect.top, NULL);
        PRInt32 oldColor;
        oldColor = ::SetTextColor(hdc, 0);
        
        ::DrawFocusRect(hdc, &widgetRect);
        ::SetTextColor(hdc, oldColor);
      }
      break;
    }
    
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
    case NS_THEME_LISTBOX:
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXTFIELD: {
      
      ::DrawEdge(hdc, &widgetRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      
      if (IsDisabled(aFrame, eventState) ||
          (aFrame->GetContent()->IsXUL() &&
           IsReadOnly(aFrame)))
        ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_BTNFACE+1));
      else
        ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_WINDOW+1));

      break;
    }
    case NS_THEME_TREEVIEW: {
      
      ::DrawEdge(hdc, &widgetRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

      
      ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_WINDOW+1));

      break;
    }
    
    case NS_THEME_TOOLTIP:
      ::FrameRect(hdc, &widgetRect, ::GetSysColorBrush(COLOR_WINDOWFRAME));
      InflateRect(&widgetRect, -1, -1);
      ::FillRect(hdc, &widgetRect, ::GetSysColorBrush(COLOR_INFOBK));

      break;
    case NS_THEME_GROUPBOX:
      ::DrawEdge(hdc, &widgetRect, EDGE_ETCHED, BF_RECT | BF_ADJUST);
      ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_BTNFACE+1));
      break;
    
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
      
      ::DrawEdge(hdc, &widgetRect, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);
      InflateRect(&widgetRect, -1, -1);
      
    case NS_THEME_TAB_PANEL:
    case NS_THEME_STATUSBAR:
    case NS_THEME_STATUSBAR_RESIZER_PANEL: {
      ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_BTNFACE+1));

      break;
    }
    
    case NS_THEME_STATUSBAR_PANEL: {
      if (aFrame->GetNextSibling())
        widgetRect.right -= 2; 

      ::DrawEdge(hdc, &widgetRect, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);

      break;
    }
    
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
      ::DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_RECT | BF_MIDDLE);

      break;
    case NS_THEME_SCALE_THUMB_VERTICAL:
    case NS_THEME_SCALE_THUMB_HORIZONTAL: {
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      ::DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_RECT | BF_SOFT | BF_MIDDLE | BF_ADJUST);
      if (IsDisabled(aFrame, eventState)) {
        DrawCheckedRect(hdc, widgetRect, COLOR_3DFACE, COLOR_3DHILIGHT,
                        (HBRUSH) COLOR_3DHILIGHT);
      }

      break;
    }
    
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL: {

      
      
      DWORD color3D, colorScrollbar, colorWindow;

      color3D = ::GetSysColor(COLOR_3DFACE);      
      colorWindow = ::GetSysColor(COLOR_WINDOW);
      colorScrollbar = ::GetSysColor(COLOR_SCROLLBAR);
      
      if ((color3D != colorScrollbar) && (colorWindow != colorScrollbar))
        
        ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_SCROLLBAR+1));
      else
      {
        DrawCheckedRect(hdc, widgetRect, COLOR_3DHILIGHT, COLOR_3DFACE,
                        (HBRUSH) COLOR_SCROLLBAR+1);
      }
      
      

      break;
    }
    
    case NS_THEME_SCALE_VERTICAL:
    case NS_THEME_SCALE_HORIZONTAL: {
      if (aWidgetType == NS_THEME_SCALE_HORIZONTAL) {
        PRInt32 adjustment = (widgetRect.bottom - widgetRect.top) / 2 - 2;
        widgetRect.top += adjustment;
        widgetRect.bottom -= adjustment;
      }
      else {
        PRInt32 adjustment = (widgetRect.right - widgetRect.left) / 2 - 2;
        widgetRect.left += adjustment;
        widgetRect.right -= adjustment;
      }

      ::DrawEdge(hdc, &widgetRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
      ::FillRect(hdc, &widgetRect, (HBRUSH) GetStockObject(GRAY_BRUSH));
 
      break;
    }
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
      ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_HIGHLIGHT+1));
      break;

    case NS_THEME_PROGRESSBAR_CHUNK: {
      nsIFrame* stateFrame = aFrame->GetParent();
      nsEventStates eventStates = GetContentState(stateFrame, aWidgetType);
      const bool indeterminate = IsIndeterminateProgress(stateFrame, eventStates);

      if (!indeterminate) {
        ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_HIGHLIGHT+1));
        break;
      }

      



      if (!QueueAnimatedContentForRefresh(aFrame->GetContent(),
                                          DEFAULT_ANIMATION_FPS)) {
        NS_WARNING("unable to animate progress widget!");
      }

      const bool vertical = IsVerticalProgress(stateFrame);
      const PRInt32 overlaySize = kProgressClassicOverlaySize;
      const double pixelsPerMillisecond = kProgressClassicIndeterminateSpeed;
      const PRInt32 frameSize = vertical ? widgetRect.bottom - widgetRect.top
                                         : widgetRect.right - widgetRect.left;
      const double interval = frameSize / pixelsPerMillisecond;
      
      double tempValue;
      double ratio = modf(PR_IntervalToMilliseconds(PR_IntervalNow())/interval,
                          &tempValue);
      PRInt32 dx = 0;

      
      
      if (!vertical && IsFrameRTL(aFrame)) {
        ratio = 1.0 - ratio;
        dx -= overlaySize;
      }
      dx += static_cast<PRInt32>(frameSize * ratio);

      RECT overlayRect = widgetRect;
      if (vertical) {
        overlayRect.bottom -= dx;
        overlayRect.top = overlayRect.bottom - overlaySize;
      } else {
        overlayRect.left += dx;
        overlayRect.right = overlayRect.left + overlaySize;
      }

      
      RECT leftoverRect = widgetRect;
      if (vertical) {
        if (overlayRect.top < widgetRect.top) {
          leftoverRect.bottom = widgetRect.bottom;
          leftoverRect.top = leftoverRect.bottom + overlayRect.top - widgetRect.top;
        }
      } else if (IsFrameRTL(aFrame)) {
        if (overlayRect.left < widgetRect.left) {
          leftoverRect.right = widgetRect.right;
          leftoverRect.left = leftoverRect.right + overlayRect.left - widgetRect.left;
        }
      } else if (overlayRect.right > widgetRect.right) {
        leftoverRect.left = widgetRect.left;
        leftoverRect.right = leftoverRect.left + overlayRect.right - widgetRect.right;
      }

      
      if (leftoverRect.top != widgetRect.top ||
          leftoverRect.left != widgetRect.left ||
          leftoverRect.right != widgetRect.right) {
        ::FillRect(hdc, &leftoverRect, (HBRUSH) (COLOR_HIGHLIGHT+1));
      }

      ::FillRect(hdc, &overlayRect, (HBRUSH) (COLOR_HIGHLIGHT+1));
      break;
    }

    
    case NS_THEME_TAB: {
      DrawTab(hdc, widgetRect,
        IsBottomTab(aFrame) ? BF_BOTTOM : BF_TOP, 
        IsSelectedTab(aFrame),
        !IsRightToSelectedTab(aFrame),
        !IsLeftToSelectedTab(aFrame));

      break;
    }
    case NS_THEME_TAB_PANELS:
      ::DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_SOFT | BF_MIDDLE |
          BF_LEFT | BF_RIGHT | BF_BOTTOM);

      break;
    case NS_THEME_MENUBAR:
      break;
    case NS_THEME_MENUPOPUP:
      NS_ASSERTION(nsUXThemeData::sFlatMenus, "Classic menus are styled entirely through CSS");
      ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_MENU+1));
      ::FrameRect(hdc, &widgetRect, ::GetSysColorBrush(COLOR_BTNSHADOW));
      break;
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM:
      
      
      if (nsUXThemeData::sFlatMenus) {
        
        if ((state & (DFCS_HOT | DFCS_PUSHED)) != 0) {
          ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_MENUHILIGHT+1));
          ::FrameRect(hdc, &widgetRect, ::GetSysColorBrush(COLOR_HIGHLIGHT));
        }
      } else {
        if (part == 1) {
          if ((state & DFCS_INACTIVE) == 0) {
            if ((state & DFCS_PUSHED) != 0) {
              ::DrawEdge(hdc, &widgetRect, BDR_SUNKENOUTER, BF_RECT);
            } else if ((state & DFCS_HOT) != 0) {
              ::DrawEdge(hdc, &widgetRect, BDR_RAISEDINNER, BF_RECT);
            }
          }
        } else {
          if ((state & (DFCS_HOT | DFCS_PUSHED)) != 0) {
            ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_HIGHLIGHT+1));
          }
        }
      }
      break;
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
      if (!(state & DFCS_CHECKED))
        break; 
    case NS_THEME_MENUARROW: {
      PRUint32 color = COLOR_MENUTEXT;
      if ((state & DFCS_INACTIVE))
        color = COLOR_GRAYTEXT;
      else if ((state & DFCS_HOT))
        color = COLOR_HIGHLIGHTTEXT;
      
      if (aWidgetType == NS_THEME_MENUCHECKBOX)
        DrawMenuImage(hdc, widgetRect, DFCS_MENUCHECK, color);
      else if (aWidgetType == NS_THEME_MENURADIO)
        DrawMenuImage(hdc, widgetRect, DFCS_MENUBULLET, color);
      else if (aWidgetType == NS_THEME_MENUARROW)
        DrawMenuImage(hdc, widgetRect, 
                      (state & DFCS_RTL) ? DFCS_MENUARROWRIGHT : DFCS_MENUARROW,
                      color);
      break;
    }
    case NS_THEME_MENUSEPARATOR: {
      
      widgetRect.left++;
      widgetRect.right--;

      
      widgetRect.top += 4;
      
      widgetRect.bottom = widgetRect.top+1;
      ::FillRect(hdc, &widgetRect, (HBRUSH)(COLOR_3DSHADOW+1));
      widgetRect.top++;
      widgetRect.bottom++;
      ::FillRect(hdc, &widgetRect, (HBRUSH)(COLOR_3DHILIGHT+1));
      break;
    }

    case NS_THEME_WINDOW_TITLEBAR:
    case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
    {
      RECT rect = widgetRect;
      PRInt32 offset = GetSystemMetrics(SM_CXFRAME);
      rect.bottom -= 1;

      
      FillRect(hdc, &rect, (HBRUSH)(COLOR_3DFACE+1));

      
      rect.top += offset;
      
      
      BOOL bFlag = TRUE;
      SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &bFlag, 0);
      if (!bFlag) {
        if (state == mozilla::widget::themeconst::FS_ACTIVE)
          FillRect(hdc, &rect, (HBRUSH)(COLOR_ACTIVECAPTION+1));
        else
          FillRect(hdc, &rect, (HBRUSH)(COLOR_INACTIVECAPTION+1));
      } else {
        DWORD startColor, endColor;
        if (state == mozilla::widget::themeconst::FS_ACTIVE) {
          startColor = GetSysColor(COLOR_ACTIVECAPTION);
          endColor = GetSysColor(COLOR_GRADIENTACTIVECAPTION);
        } else {
          startColor = GetSysColor(COLOR_INACTIVECAPTION);
          endColor = GetSysColor(COLOR_GRADIENTINACTIVECAPTION);
        }

        TRIVERTEX vertex[2];
        vertex[0].x     = rect.left;
        vertex[0].y     = rect.top;
        vertex[0].Red   = GetRValue(startColor) << 8;
        vertex[0].Green = GetGValue(startColor) << 8;
        vertex[0].Blue  = GetBValue(startColor) << 8;
        vertex[0].Alpha = 0;

        vertex[1].x     = rect.right;
        vertex[1].y     = rect.bottom; 
        vertex[1].Red   = GetRValue(endColor) << 8;
        vertex[1].Green = GetGValue(endColor) << 8;
        vertex[1].Blue  = GetBValue(endColor) << 8;
        vertex[1].Alpha = 0;

        GRADIENT_RECT gRect;
        gRect.UpperLeft  = 0;
        gRect.LowerRight = 1;
        
        GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
      }

      if (aWidgetType == NS_THEME_WINDOW_TITLEBAR) {
        
        DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_TOP);
      }
      break;
    }

    case NS_THEME_WINDOW_FRAME_LEFT:
      DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_LEFT);
      break;

    case NS_THEME_WINDOW_FRAME_RIGHT:
      DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_RIGHT);
      break;

    case NS_THEME_WINDOW_FRAME_BOTTOM:
      DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_BOTTOM);
      break;

    case NS_THEME_WINDOW_BUTTON_CLOSE:
    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
    case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
    case NS_THEME_WINDOW_BUTTON_RESTORE:
    {
      if (aWidgetType == NS_THEME_WINDOW_BUTTON_MINIMIZE) {
        OffsetBackgroundRect(widgetRect, CAPTIONBUTTON_MINIMIZE);
      }
      else if (aWidgetType == NS_THEME_WINDOW_BUTTON_MAXIMIZE ||
               aWidgetType == NS_THEME_WINDOW_BUTTON_RESTORE) {
        OffsetBackgroundRect(widgetRect, CAPTIONBUTTON_RESTORE);
      }
      else if (aWidgetType == NS_THEME_WINDOW_BUTTON_CLOSE) {
        OffsetBackgroundRect(widgetRect, CAPTIONBUTTON_CLOSE);
      }
      PRInt32 oldTA = SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
      DrawFrameControl(hdc, &widgetRect, part, state);
      SetTextAlign(hdc, oldTA);
      break;
    }

    default:
      rv = NS_ERROR_FAILURE;
      break;
  }

  nativeDrawing.EndNativeDrawing();

  if (NS_FAILED(rv))
    return rv;

  if (nativeDrawing.ShouldRenderAgain())
    goto RENDER_AGAIN;

  nativeDrawing.PaintToContext();

  return rv;
}

PRUint32
nsNativeThemeWin::GetWidgetNativeDrawingFlags(PRUint8 aWidgetType)
{
  switch (aWidgetType) {
    case NS_THEME_BUTTON:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:

    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXTFIELD:
      return
        gfxWindowsNativeDrawing::CANNOT_DRAW_TO_COLOR_ALPHA |
        gfxWindowsNativeDrawing::CAN_AXIS_ALIGNED_SCALE |
        gfxWindowsNativeDrawing::CANNOT_COMPLEX_TRANSFORM;

    
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCALE_HORIZONTAL:
    case NS_THEME_SCALE_VERTICAL:
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL:
    case NS_THEME_SPINNER_UP_BUTTON:
    case NS_THEME_SPINNER_DOWN_BUTTON:
    case NS_THEME_LISTBOX:
    case NS_THEME_TREEVIEW:
    case NS_THEME_TOOLTIP:
    case NS_THEME_STATUSBAR:
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_RESIZER:
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    case NS_THEME_TAB:
    case NS_THEME_TAB_PANEL:
    case NS_THEME_TAB_PANELS:
    case NS_THEME_MENUBAR:
    case NS_THEME_MENUPOPUP:
    case NS_THEME_MENUITEM:
      break;

    
    
    
    case NS_THEME_DROPDOWN_BUTTON:
    
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO:
    case NS_THEME_GROUPBOX:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM:
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
    case NS_THEME_MENUARROW:
      return
        gfxWindowsNativeDrawing::CANNOT_DRAW_TO_COLOR_ALPHA |
        gfxWindowsNativeDrawing::CANNOT_AXIS_ALIGNED_SCALE |
        gfxWindowsNativeDrawing::CANNOT_COMPLEX_TRANSFORM;
  }

  return
    gfxWindowsNativeDrawing::CANNOT_DRAW_TO_COLOR_ALPHA |
    gfxWindowsNativeDrawing::CANNOT_AXIS_ALIGNED_SCALE |
    gfxWindowsNativeDrawing::CANNOT_COMPLEX_TRANSFORM;
}






extern bool gDisableNativeTheme;

nsresult NS_NewNativeTheme(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (gDisableNativeTheme)
    return NS_ERROR_NO_INTERFACE;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsNativeThemeWin* theme = new nsNativeThemeWin();
  if (!theme)
    return NS_ERROR_OUT_OF_MEMORY;
  return theme->QueryInterface(aIID, aResult);
}
