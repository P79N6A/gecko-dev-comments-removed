




#include "nsNativeThemeWin.h"
#include "mozilla/WindowsVersion.h"
#include "nsRenderingContext.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsTransform2D.h"
#include "nsThemeConstants.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsEventStates.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsLookAndFeel.h"
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
#include "gfxWindowsNativeDrawing.h"

#include "nsUXThemeData.h"
#include "nsUXThemeConstants.h"
#include <algorithm>

using mozilla::IsVistaOrLater;
using namespace mozilla::widget;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gWindowsLog;
#endif

NS_IMPL_ISUPPORTS_INHERITED1(nsNativeThemeWin, nsNativeTheme, nsITheme)

nsNativeThemeWin::nsNativeThemeWin() :
  mProgressDeterminateTimeStamp(TimeStamp::Now()),
  mProgressIndeterminateTimeStamp(TimeStamp::Now())
{
  
  
  
}

nsNativeThemeWin::~nsNativeThemeWin()
{
  nsUXThemeData::Invalidate();
}

static int32_t
GetTopLevelWindowActiveState(nsIFrame *aFrame)
{
  
  
  nsIWidget* widget = aFrame->GetNearestWidget();
  nsWindowBase * window = static_cast<nsWindowBase*>(widget);
  if (!window)
    return mozilla::widget::themeconst::FS_INACTIVE;
  if (widget && !window->IsTopLevelWidget() &&
      !(window = window->GetParentWindowBase(false)))
    return mozilla::widget::themeconst::FS_INACTIVE;

  if (window->GetWindowHandle() == ::GetActiveWindow())
    return mozilla::widget::themeconst::FS_ACTIVE;
  return mozilla::widget::themeconst::FS_INACTIVE;
}

static int32_t
GetWindowFrameButtonState(nsIFrame *aFrame, nsEventStates eventState)
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

static int32_t
GetClassicWindowFrameButtonState(nsEventStates eventState)
{
  if (eventState.HasState(NS_EVENT_STATE_ACTIVE) &&
      eventState.HasState(NS_EVENT_STATE_HOVER))
    return DFCS_BUTTONPUSH|DFCS_PUSHED;
  return DFCS_BUTTONPUSH;
}

static bool
IsTopLevelMenu(nsIFrame *aFrame)
{
  bool isTopLevel(false);
  nsMenuFrame *menuFrame = do_QueryFrame(aFrame);
  if (menuFrame) {
    isTopLevel = menuFrame->IsOnMenuBar();
  }
  return isTopLevel;
}

static MARGINS
GetCheckboxMargins(HANDLE theme, HDC hdc)
{
    MARGINS checkboxContent = {0};
    GetThemeMargins(theme, hdc, MENU_POPUPCHECK, MCB_NORMAL,
                    TMT_CONTENTMARGINS, nullptr, &checkboxContent);
    return checkboxContent;
}

static SIZE
GetCheckboxBGSize(HANDLE theme, HDC hdc)
{
    SIZE checkboxSize;
    GetThemePartSize(theme, hdc, MENU_POPUPCHECK, MC_CHECKMARKNORMAL,
                     nullptr, TS_TRUE, &checkboxSize);

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

static SIZE
GetCheckboxBGBounds(HANDLE theme, HDC hdc)
{
    MARGINS checkboxBGSizing = {0};
    MARGINS checkboxBGContent = {0};
    GetThemeMargins(theme, hdc, MENU_POPUPCHECKBACKGROUND, MCB_NORMAL,
                    TMT_SIZINGMARGINS, nullptr, &checkboxBGSizing);
    GetThemeMargins(theme, hdc, MENU_POPUPCHECKBACKGROUND, MCB_NORMAL,
                    TMT_CONTENTMARGINS, nullptr, &checkboxBGContent);

#define posdx(d) ((d) > 0 ? d : 0)

    int dx = posdx(checkboxBGContent.cxRightWidth -
                   checkboxBGSizing.cxRightWidth) +
             posdx(checkboxBGContent.cxLeftWidth -
                   checkboxBGSizing.cxLeftWidth);
    int dy = posdx(checkboxBGContent.cyTopHeight -
                   checkboxBGSizing.cyTopHeight) +
             posdx(checkboxBGContent.cyBottomHeight -
                   checkboxBGSizing.cyBottomHeight);

#undef posdx

    SIZE ret(GetCheckboxBGSize(theme, hdc));
    ret.cx += dx;
    ret.cy += dy;
    return ret;
}

static SIZE
GetGutterSize(HANDLE theme, HDC hdc)
{
    SIZE gutterSize;
    GetThemePartSize(theme, hdc, MENU_POPUPGUTTER, 0, nullptr, TS_TRUE, &gutterSize);

    SIZE checkboxBGSize(GetCheckboxBGBounds(theme, hdc));

    SIZE itemSize;
    GetThemePartSize(theme, hdc, MENU_POPUPITEM, MPI_NORMAL, nullptr, TS_TRUE, &itemSize);

    
    double scaleFactor = nsIWidget::DefaultScaleOverride();
    if (scaleFactor <= 0.0) {
      scaleFactor = gfxWindowsPlatform::GetPlatform()->GetDPIScale();
    }
    int iconDevicePixels = NSToIntRound(16 * scaleFactor);
    SIZE iconSize = {
      iconDevicePixels, iconDevicePixels
    };
    
    MARGINS margins = {0};
    GetThemeMargins(theme, hdc, MENU_POPUPCHECKBACKGROUND, MCB_NORMAL,
                    TMT_CONTENTMARGINS, nullptr, &margins);
    iconSize.cx += margins.cxLeftWidth + margins.cxRightWidth;
    iconSize.cy += margins.cyTopHeight + margins.cyBottomHeight;

    int width = std::max(itemSize.cx, std::max(iconSize.cx, checkboxBGSize.cx) + gutterSize.cx);
    int height = std::max(itemSize.cy, std::max(iconSize.cy, checkboxBGSize.cy));

    SIZE ret;
    ret.cx = width;
    ret.cy = height;
    return ret;
}




















static HRESULT
DrawThemeBGRTLAware(HANDLE aTheme, HDC aHdc, int aPart, int aState,
                    const RECT *aWidgetRect, const RECT *aClipRect,
                    bool aIsRtl)
{
  NS_ASSERTION(aTheme, "Bad theme handle.");
  NS_ASSERTION(aHdc, "Bad hdc.");
  NS_ASSERTION(aWidgetRect, "Bad rect.");
  NS_ASSERTION(aClipRect, "Bad clip rect.");

  if (!aIsRtl) {
    return DrawThemeBackground(aTheme, aHdc, aPart, aState,
                               aWidgetRect, aClipRect);
  }

  HGDIOBJ hObj = GetCurrentObject(aHdc, OBJ_BITMAP);
  BITMAP bitmap;
  POINT vpOrg;

  if (hObj && GetObject(hObj, sizeof(bitmap), &bitmap) &&
      GetViewportOrgEx(aHdc, &vpOrg)) {
    RECT newWRect(*aWidgetRect);
    newWRect.left = bitmap.bmWidth - (aWidgetRect->right + 2*vpOrg.x);
    newWRect.right = bitmap.bmWidth - (aWidgetRect->left + 2*vpOrg.x);

    RECT newCRect;
    RECT *newCRectPtr = nullptr;

    if (aClipRect) {
      newCRect.top = aClipRect->top;
      newCRect.bottom = aClipRect->bottom;
      newCRect.left = bitmap.bmWidth - (aClipRect->right + 2*vpOrg.x);
      newCRect.right = bitmap.bmWidth - (aClipRect->left + 2*vpOrg.x);
      newCRectPtr = &newCRect;
    }

    SetLayout(aHdc, LAYOUT_RTL);
    HRESULT hr = DrawThemeBackground(aTheme, aHdc, aPart, aState, &newWRect,
                                     newCRectPtr);
    SetLayout(aHdc, 0);
    if (SUCCEEDED(hr)) {
      return hr;
    }
  }
  return DrawThemeBackground(aTheme, aHdc, aPart, aState,
                             aWidgetRect, aClipRect);
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


static void
AddPaddingRect(nsIntSize* aSize, CaptionButton button) {
  if (!aSize)
    return;
  RECT offset;
  if (!IsAppThemed())
    offset = buttonData[CAPTION_CLASSIC].hotPadding[button];
  else if (!IsVistaOrLater())
    offset = buttonData[CAPTION_XPTHEME].hotPadding[button];
  else
    offset = buttonData[CAPTION_BASIC].hotPadding[button];
  aSize->width += offset.left + offset.right;
  aSize->height += offset.top + offset.bottom;
}



static void
OffsetBackgroundRect(RECT& rect, CaptionButton button) {
  RECT offset;
  if (!IsAppThemed())
    offset = buttonData[CAPTION_CLASSIC].hotPadding[button];
  else if (!IsVistaOrLater())
    offset = buttonData[CAPTION_XPTHEME].hotPadding[button];
  else
    offset = buttonData[CAPTION_BASIC].hotPadding[button];
  rect.left += offset.left;
  rect.top += offset.top;
  rect.right -= offset.right;
  rect.bottom -= offset.bottom;
}


































static const double kProgressDeterminateTimeSpan = 3.0;
static const double kProgressIndeterminateTimeSpan = 5.0;

static const int32_t kProgressHorizontalVistaOverlaySize = 120;

static const int32_t kProgressHorizontalXPOverlaySize = 55;

static const int32_t kProgressVerticalOverlaySize = 45;

static const int32_t kProgressVerticalIndeterminateOverlaySize = 60;

static const int32_t kProgressClassicOverlaySize = 40;





static int32_t
GetProgressOverlayStyle(bool aIsVertical)
{ 
  if (aIsVertical) {
    if (IsVistaOrLater()) {
      return PP_MOVEOVERLAYVERT;
    }
    return PP_CHUNKVERT;
  } else {
    if (IsVistaOrLater()) {
      return PP_MOVEOVERLAY;
    }
    return PP_CHUNK;
  }
}






static int32_t
GetProgressOverlaySize(bool aIsVertical, bool aIsIndeterminate)
{
  if (IsVistaOrLater()) {
    if (aIsVertical) {
      return aIsIndeterminate ? kProgressVerticalIndeterminateOverlaySize
                              : kProgressVerticalOverlaySize;
    }
    return kProgressHorizontalVistaOverlaySize;
  }
  return kProgressHorizontalXPOverlaySize;
}





static bool
IsProgressMeterFilled(nsIFrame* aFrame)
{
  NS_ENSURE_TRUE(aFrame, false);
  nsIFrame* parentFrame = aFrame->GetParent();
  NS_ENSURE_TRUE(parentFrame, false);
  return nsNativeTheme::GetProgressValue(parentFrame) ==
         nsNativeTheme::GetProgressMaxValue(parentFrame);
}







RECT
nsNativeThemeWin::CalculateProgressOverlayRect(nsIFrame* aFrame,
                                               RECT* aWidgetRect,
                                               bool aIsVertical,
                                               bool aIsIndeterminate,
                                               bool aIsClassic)
{
  NS_ASSERTION(aFrame, "bad frame pointer");
  NS_ASSERTION(aWidgetRect, "bad rect pointer");

  int32_t frameSize = aIsVertical ? aWidgetRect->bottom - aWidgetRect->top
                                  : aWidgetRect->right - aWidgetRect->left;

  
  
  double span = aIsIndeterminate ? kProgressIndeterminateTimeSpan
                                 : kProgressDeterminateTimeSpan;
  TimeDuration period;
  if (!aIsIndeterminate) {
    if (TimeStamp::Now() > (mProgressDeterminateTimeStamp +
                            TimeDuration::FromSeconds(span))) {
      mProgressDeterminateTimeStamp = TimeStamp::Now();
    }
    period = TimeStamp::Now() - mProgressDeterminateTimeStamp;
  } else {
    if (TimeStamp::Now() > (mProgressIndeterminateTimeStamp +
                            TimeDuration::FromSeconds(span))) {
      mProgressIndeterminateTimeStamp = TimeStamp::Now();
    }
    period = TimeStamp::Now() - mProgressIndeterminateTimeStamp;
  }

  double percent = period / TimeDuration::FromSeconds(span);

  if (!aIsVertical && IsFrameRTL(aFrame))
    percent = 1 - percent;

  RECT overlayRect = *aWidgetRect;
  int32_t overlaySize;
  if (!aIsClassic) {
    overlaySize = GetProgressOverlaySize(aIsVertical, aIsIndeterminate);
  } else {
    overlaySize = kProgressClassicOverlaySize;
  } 

  
  
  
  
  
  
  int trackWidth = frameSize > overlaySize ? frameSize : overlaySize;
  if (!aIsVertical) {
    int xPos = aWidgetRect->left - trackWidth;
    xPos += (int)ceil(((double)(trackWidth*2) * percent));
    overlayRect.left = xPos;
    overlayRect.right = xPos + overlaySize;
  } else {
    int yPos = aWidgetRect->bottom + trackWidth;
    yPos -= (int)ceil(((double)(trackWidth*2) * percent));
    overlayRect.bottom = yPos;
    overlayRect.top = yPos - overlaySize;
  }
  return overlayRect;
}

















static void
DrawChunkProgressMeter(HTHEME aTheme, HDC aHdc, int aPart,
                       int aState, nsIFrame* aFrame, RECT* aWidgetRect,
                       RECT* aClipRect, gfxFloat aAppUnits, bool aIsIndeterm,
                       bool aIsVertical, bool aIsRtl)
{
  NS_ASSERTION(aTheme, "Bad theme.");
  NS_ASSERTION(aHdc, "Bad hdc.");
  NS_ASSERTION(aWidgetRect, "Bad rect.");
  NS_ASSERTION(aClipRect, "Bad clip rect.");
  NS_ASSERTION(aFrame, "Bad frame.");

  
  
  
  if (aIsVertical) {
    DrawThemeBackground(aTheme, aHdc, aPart, aState, aWidgetRect, aClipRect);
    return;
  }

  
  int chunkSize, spaceSize;
  if (FAILED(GetThemeMetric(aTheme, aHdc, aPart, aState,
                            TMT_PROGRESSCHUNKSIZE, &chunkSize)) ||
      FAILED(GetThemeMetric(aTheme, aHdc, aPart, aState,
                            TMT_PROGRESSSPACESIZE, &spaceSize))) {
    DrawThemeBackground(aTheme, aHdc, aPart, aState, aWidgetRect, aClipRect);
    return;
  }

  
  if (!aIsRtl || aIsIndeterm) {
    for (int chunk = aWidgetRect->left; chunk <= aWidgetRect->right;
         chunk += (chunkSize+spaceSize)) {
      if (!aIsIndeterm && ((chunk + chunkSize) > aWidgetRect->right)) {
        
        
        
        
        
        
        if (!IsProgressMeterFilled(aFrame)) {
          break;
        }
      }
      RECT bounds =
        { chunk, aWidgetRect->top, chunk + chunkSize, aWidgetRect->bottom };
      DrawThemeBackground(aTheme, aHdc, aPart, aState, &bounds, aClipRect);
    }
  } else {
    
    for (int chunk = aWidgetRect->right; chunk >= aWidgetRect->left;
         chunk -= (chunkSize+spaceSize)) {
      if ((chunk - chunkSize) < aWidgetRect->left) {
        if (!IsProgressMeterFilled(aFrame)) {
          break;
        }
      }
      RECT bounds =
        { chunk - chunkSize, aWidgetRect->top, chunk, aWidgetRect->bottom };
      DrawThemeBackground(aTheme, aHdc, aPart, aState, &bounds, aClipRect);
    }
  }
}
















void
nsNativeThemeWin::DrawThemedProgressMeter(nsIFrame* aFrame, int aWidgetType,
                                          HANDLE aTheme, HDC aHdc,
                                          int aPart, int aState,
                                          RECT* aWidgetRect, RECT* aClipRect,
                                          gfxFloat aAppUnits)
{
  if (!aFrame || !aTheme || !aHdc)
    return;

  NS_ASSERTION(aWidgetRect, "bad rect pointer");
  NS_ASSERTION(aClipRect, "bad clip rect pointer");

  RECT adjWidgetRect, adjClipRect;
  adjWidgetRect = *aWidgetRect;
  adjClipRect = *aClipRect;
  if (!IsVistaOrLater()) {
    
    
    InflateRect(&adjWidgetRect, 1, 1);
    InflateRect(&adjClipRect, 1, 1);
  }

  nsIFrame* parentFrame = aFrame->GetParent();
  if (!parentFrame) {
    
    NS_WARNING("No parent frame for progress rendering. Can't paint.");
    return;
  }

  nsEventStates eventStates = GetContentState(parentFrame, aWidgetType);
  bool vertical = IsVerticalProgress(parentFrame) ||
                  aWidgetType == NS_THEME_PROGRESSBAR_CHUNK_VERTICAL;
  bool indeterminate = IsIndeterminateProgress(parentFrame, eventStates);
  bool animate = indeterminate;

  if (IsVistaOrLater()) {
    
    
    DrawThemeBackground(aTheme, aHdc, aPart, aState,
                        &adjWidgetRect, &adjClipRect);
    if (!IsProgressMeterFilled(aFrame)) {
      animate = true;
    }
  } else if (!indeterminate) {
    
    DrawChunkProgressMeter(aTheme, aHdc, aPart, aState, aFrame,
                           &adjWidgetRect, &adjClipRect, aAppUnits,
                           indeterminate, vertical, IsFrameRTL(aFrame));
  }    

  if (animate) {
    
    int32_t overlayPart = GetProgressOverlayStyle(vertical);
    RECT overlayRect =
      CalculateProgressOverlayRect(aFrame, &adjWidgetRect, vertical,
                                   indeterminate, false);
    if (IsVistaOrLater()) {
      DrawThemeBackground(aTheme, aHdc, overlayPart, aState, &overlayRect,
                          &adjClipRect);
    } else {
      DrawChunkProgressMeter(aTheme, aHdc, overlayPart, aState, aFrame,
                             &overlayRect, &adjClipRect, aAppUnits,
                             indeterminate, vertical, IsFrameRTL(aFrame));
    }

    if (!QueueAnimatedContentForRefresh(aFrame->GetContent(), 60)) {
      NS_WARNING("unable to animate progress widget!");
    }
  }
}

HANDLE
nsNativeThemeWin::GetTheme(uint8_t aWidgetType)
{ 
  if (!IsVistaOrLater()) {
    
    
    
    
    if (aWidgetType == NS_THEME_DROPDOWN)
      aWidgetType = NS_THEME_TEXTFIELD;
  }

  switch (aWidgetType) {
    case NS_THEME_BUTTON:
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX:
    case NS_THEME_GROUPBOX:
      return nsUXThemeData::GetTheme(eUXButton);
    case NS_THEME_NUMBER_INPUT:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
      return nsUXThemeData::GetTheme(eUXEdit);
    case NS_THEME_TOOLTIP:
      
      return !IsVistaOrLater() ?
        nullptr : nsUXThemeData::GetTheme(eUXTooltip);
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
    case NS_THEME_RANGE:
    case NS_THEME_RANGE_THUMB:
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
  return nullptr;
}

int32_t
nsNativeThemeWin::StandardGetState(nsIFrame* aFrame, uint8_t aWidgetType,
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
nsNativeThemeWin::IsMenuActive(nsIFrame *aFrame, uint8_t aWidgetType)
{
  nsIContent* content = aFrame->GetContent();
  if (content->IsXUL() &&
      content->NodeInfo()->Equals(nsGkAtoms::richlistitem))
    return CheckBooleanAttr(aFrame, nsGkAtoms::selected);

  return CheckBooleanAttr(aFrame, nsGkAtoms::menuactive);
}








nsresult 
nsNativeThemeWin::GetThemePartAndState(nsIFrame* aFrame, uint8_t aWidgetType, 
                                       int32_t& aPart, int32_t& aState)
{
  if (!IsVistaOrLater()) {
    
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
    case NS_THEME_NUMBER_INPUT:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE: {
      nsEventStates eventState = GetContentState(aFrame, aWidgetType);

      if (IsVistaOrLater()) {
        







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
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL: {
      
      
      
      bool vertical = IsVerticalProgress(aFrame) ||
                      aWidgetType == NS_THEME_PROGRESSBAR_VERTICAL;
      aPart = vertical ? PP_BARVERT : PP_BAR;
      aState = PBBS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL: {
      nsIFrame* parentFrame = aFrame->GetParent();
      nsEventStates eventStates = GetContentState(parentFrame, aWidgetType);
      if (aWidgetType == NS_THEME_PROGRESSBAR_CHUNK_VERTICAL ||
          IsVerticalProgress(parentFrame)) {
        aPart = IsVistaOrLater() ?
          PP_FILLVERT : PP_CHUNKVERT;
      } else {
        aPart = IsVistaOrLater() ?
          PP_FILL : PP_CHUNK;
      }

      aState = PBBVS_NORMAL;
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
        nsEventStates parentState = GetContentState(parent, parent->StyleDisplay()->mAppearance);
        if (eventState.HasAllStates(NS_EVENT_STATE_HOVER | NS_EVENT_STATE_ACTIVE))
          aState += TS_ACTIVE;
        else if (eventState.HasState(NS_EVENT_STATE_HOVER))
          aState += TS_HOVER;
        else if (IsVistaOrLater() &&
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
    case NS_THEME_RANGE:
    case NS_THEME_SCALE_HORIZONTAL:
    case NS_THEME_SCALE_VERTICAL: {
      if (aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
          (aWidgetType == NS_THEME_RANGE &&
           IsRangeHorizontal(aFrame))) {
        aPart = TKP_TRACK;
        aState = TRS_NORMAL;
      } else {
        aPart = TKP_TRACKVERT;
        aState = TRVS_NORMAL;
      }
      return NS_OK;
    }
    case NS_THEME_RANGE_THUMB:
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL: {
      if (aWidgetType == NS_THEME_RANGE_THUMB) {
        if (IsRangeHorizontal(aFrame)) {
          aPart = TKP_THUMBBOTTOM;
        } else {
          aPart = IsFrameRTL(aFrame) ? TKP_THUMBLEFT : TKP_THUMBRIGHT;
        }
      } else {
        aPart = (aWidgetType == NS_THEME_SCALE_THUMB_HORIZONTAL) ?
                TKP_THUMB : TKP_THUMBVERT;
      }
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
      if (IsVistaOrLater()) {
        
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
      aPart = IsVistaOrLater() ?
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

      if (IsVistaOrLater()) {
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

static bool
AssumeThemePartAndStateAreTransparent(int32_t aPart, int32_t aState)
{
  if (aPart == MENU_POPUPITEM && aState == MBI_NORMAL) {
    return true;
  }
  return false;
}

NS_IMETHODIMP
nsNativeThemeWin::DrawWidgetBackground(nsRenderingContext* aContext,
                                       nsIFrame* aFrame,
                                       uint8_t aWidgetType,
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

  int32_t part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  if (AssumeThemePartAndStateAreTransparent(part, state)) {
    return NS_OK;
  }

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
      
      
      
      int32_t edgeSize = 2;
    
      
      
      
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

  
  
  
  if (aWidgetType == NS_THEME_RANGE ||
      aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
      aWidgetType == NS_THEME_SCALE_VERTICAL) {
    RECT contentRect;
    GetThemeBackgroundContentRect(theme, hdc, part, state, &widgetRect, &contentRect);

    SIZE siz;
    GetThemePartSize(theme, hdc, part, state, &widgetRect, TS_TRUE, &siz);

    
    
    if (aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
        (aWidgetType == NS_THEME_RANGE && IsRangeHorizontal(aFrame))) {
      contentRect.top += (contentRect.bottom - contentRect.top - siz.cy) / 2;
      contentRect.bottom = contentRect.top + siz.cy;
    }
    else {
      if (!IsFrameRTL(aFrame)) {
        contentRect.left += (contentRect.right - contentRect.left - siz.cx) / 2;
        contentRect.right = contentRect.left + siz.cx;
      } else {
        contentRect.right -= (contentRect.right - contentRect.left - siz.cx) / 2;
        contentRect.left = contentRect.right - siz.cx;
      }
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
    GetThemePartSize(theme, hdc, MENU_POPUPBORDERS, 0, nullptr, TS_TRUE, &borderSize);

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
  else if (aWidgetType == NS_THEME_PROGRESSBAR ||
           aWidgetType == NS_THEME_PROGRESSBAR_VERTICAL) {
    
    
    
    COLORREF color;
    color = GetPixel(hdc, widgetRect.left, widgetRect.top);
    DrawThemeBackground(theme, hdc, part, state, &widgetRect, &clipRect);
    SetPixel(hdc, widgetRect.left, widgetRect.top, color);
    SetPixel(hdc, widgetRect.right-1, widgetRect.top, color);
    SetPixel(hdc, widgetRect.right-1, widgetRect.bottom-1, color);
    SetPixel(hdc, widgetRect.left, widgetRect.bottom-1, color);
  }
  else if (aWidgetType == NS_THEME_PROGRESSBAR_CHUNK ||
           aWidgetType == NS_THEME_PROGRESSBAR_CHUNK_VERTICAL) {
    DrawThemedProgressMeter(aFrame, aWidgetType, theme, hdc, part, state,
                            &widgetRect, &clipRect, p2a);
  }
  
  
  else if (part >= 0) {
    DrawThemeBackground(theme, hdc, part, state, &widgetRect, &clipRect);
  }

  
  
  if (((aWidgetType == NS_THEME_CHECKBOX || aWidgetType == NS_THEME_RADIO) &&
        aFrame->GetContent()->IsHTML()) ||
      aWidgetType == NS_THEME_RANGE ||
      aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
      aWidgetType == NS_THEME_SCALE_VERTICAL) {
      nsEventStates contentState = GetContentState(aFrame, aWidgetType);

      if (contentState.HasState(NS_EVENT_STATE_FOCUS)) {
        POINT vpOrg;
        HPEN hPen = nullptr;

        uint8_t id = SaveDC(hdc);

        ::SelectClipRgn(hdc, nullptr);
        ::GetViewportOrgEx(hdc, &vpOrg);
        ::SetBrushOrgEx(hdc, vpOrg.x + widgetRect.left, vpOrg.y + widgetRect.top, nullptr);

        
        
        if (IsVistaOrLater() &&
            aWidgetType == NS_THEME_CHECKBOX) {
          LOGBRUSH lb;
          lb.lbStyle = BS_SOLID;
          lb.lbColor = RGB(255,255,255);
          lb.lbHatch = 0;

          hPen = ::ExtCreatePen(PS_COSMETIC|PS_ALTERNATE, 1, &lb, 0, nullptr);
          ::SelectObject(hdc, hPen);

          
          if (contentState.HasState(NS_EVENT_STATE_ACTIVE)) {
            ::MoveToEx(hdc, widgetRect.left, widgetRect.bottom-1, nullptr);
            ::LineTo(hdc, widgetRect.left, widgetRect.top);
            ::LineTo(hdc, widgetRect.right-1, widgetRect.top);
          }

          
          ::MoveToEx(hdc, widgetRect.right-1, widgetRect.top, nullptr);
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
    DrawThemeEdge(theme, hdc, RP_BAND, 0, &widgetRect, EDGE_ETCHED, BF_TOP, nullptr);
  }
  else if (aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL ||
           aWidgetType == NS_THEME_SCROLLBAR_THUMB_VERTICAL)
  {
    

    SIZE gripSize;
    MARGINS thumbMgns;
    int gripPart = (aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL) ?
                   SP_GRIPPERHOR : SP_GRIPPERVERT;

    if (GetThemePartSize(theme, hdc, gripPart, state, nullptr, TS_TRUE, &gripSize) == S_OK &&
        GetThemeMargins(theme, hdc, part, state, TMT_CONTENTMARGINS, nullptr, &thumbMgns) == S_OK &&
        gripSize.cx + thumbMgns.cxLeftWidth + thumbMgns.cxRightWidth <= widgetRect.right - widgetRect.left &&
        gripSize.cy + thumbMgns.cyTopHeight + thumbMgns.cyBottomHeight <= widgetRect.bottom - widgetRect.top)
    {
      DrawThemeBackground(theme, hdc, gripPart, state, &widgetRect, &clipRect);
    }
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
                                  uint8_t aWidgetType,
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

  int32_t part, state;
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
  HRESULT res = GetThemeBackgroundContentRect(theme, nullptr, part, state, &outerRect, &contentRect);
  
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

  if (aFrame && (aWidgetType == NS_THEME_NUMBER_INPUT ||
                 aWidgetType == NS_THEME_TEXTFIELD ||
                 aWidgetType == NS_THEME_TEXTFIELD_MULTILINE)) {
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
                                   uint8_t aWidgetType,
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
      aResult->top = GetSystemMetrics(SM_CXFRAME)
                   + GetSystemMetrics(SM_CXPADDEDBORDER);
    return true;
  }

  if (!theme)
    return ClassicGetWidgetPadding(aContext, aFrame, aWidgetType, aResult);

  if (aWidgetType == NS_THEME_MENUPOPUP)
  {
    SIZE popupSize;
    GetThemePartSize(theme, nullptr, MENU_POPUPBORDERS,  0, nullptr, TS_TRUE, &popupSize);
    aResult->top = aResult->bottom = popupSize.cy;
    aResult->left = aResult->right = popupSize.cx;
    return true;
  }

  if (IsVistaOrLater()) {
    if (aWidgetType == NS_THEME_NUMBER_INPUT ||
        aWidgetType == NS_THEME_TEXTFIELD ||
        aWidgetType == NS_THEME_TEXTFIELD_MULTILINE ||
        aWidgetType == NS_THEME_DROPDOWN)
    {
      
      if (aFrame->PresContext()->HasAuthorSpecifiedRules(aFrame, NS_AUTHOR_SPECIFIED_PADDING))
        return false;
    }

    





    if (aWidgetType == NS_THEME_NUMBER_INPUT ||
        aWidgetType == NS_THEME_TEXTFIELD ||
        aWidgetType == NS_THEME_TEXTFIELD_MULTILINE) {
      aResult->top = aResult->bottom = 2;
      aResult->left = aResult->right = 2;
      return true;
    } else if (IsHTMLContent(aFrame) && aWidgetType == NS_THEME_DROPDOWN) {
      




      aResult->top = aResult->bottom = 1;
      aResult->left = aResult->right = 1;
      return true;
    }
  }

  int32_t right, left, top, bottom;
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
          SIZE size(GetGutterSize(theme, nullptr));
          left = size.cx + 2;
        }
        break;
    case NS_THEME_MENUSEPARATOR:
        {
          SIZE size(GetGutterSize(theme, nullptr));
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
                                    uint8_t aOverflowRect,
                                    nsRect* aResult)
{
  






#if 0
  if (IsVistaOrLater()) {
    



    if (aWidgetType == NS_THEME_DROPDOWN_BUTTON &&
        IsHTMLContent(aFrame) &&
        !IsWidgetStyled(aFrame->GetParent()->PresContext(),
                        aFrame->GetParent(),
                        NS_THEME_DROPDOWN))
    {
      int32_t p2a = aContext->AppUnitsPerDevPixel();
      
      nsMargin m(p2a, p2a, p2a, 0);
      aOverflowRect->Inflate (m);
      return true;
    }
  }
#endif

  return false;
}

NS_IMETHODIMP
nsNativeThemeWin::GetMinimumWidgetSize(nsRenderingContext* aContext, nsIFrame* aFrame,
                                       uint8_t aWidgetType,
                                       nsIntSize* aResult, bool* aIsOverridable)
{
  (*aResult).width = (*aResult).height = 0;
  *aIsOverridable = true;

  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return ClassicGetMinimumWidgetSize(aContext, aFrame, aWidgetType, aResult, aIsOverridable);

  switch (aWidgetType) {
    case NS_THEME_GROUPBOX:
    case NS_THEME_NUMBER_INPUT:
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
        SIZE gutterSize(GetGutterSize(theme, nullptr));
        aResult->width = gutterSize.cx;
        aResult->height = gutterSize.cy;
        return NS_OK;
      }
      break;

    case NS_THEME_MENUIMAGE:
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
      {
        SIZE boxSize(GetGutterSize(theme, nullptr));
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

    case NS_THEME_RANGE_THUMB:
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL:
    {
      *aIsOverridable = false;
      
      
      if (IsVistaOrLater()) {
        if (aWidgetType == NS_THEME_SCALE_THUMB_HORIZONTAL ||
            (aWidgetType == NS_THEME_RANGE_THUMB && IsRangeHorizontal(aFrame))) {
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

    case NS_THEME_SCROLLBAR:
    {
      if (nsLookAndFeel::GetInt(
            nsLookAndFeel::eIntID_UseOverlayScrollbars) != 0) {
        aResult->SizeTo(::GetSystemMetrics(SM_CXHSCROLL),
                        ::GetSystemMetrics(SM_CYVSCROLL));
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
      
      
      
      aResult->width = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_RESTORE].cx;
      aResult->height = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_RESTORE].cy;
      
      if (!IsVistaOrLater()) {
        aResult->width -= 4;
        aResult->height -= 4;
      }
      AddPaddingRect(aResult, CAPTIONBUTTON_RESTORE);
      *aIsOverridable = false;
      return NS_OK;

    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
      aResult->width = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_MINIMIZE].cx;
      aResult->height = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_MINIMIZE].cy;
      if (!IsVistaOrLater()) {
        aResult->width -= 4;
        aResult->height -= 4;
      }
      AddPaddingRect(aResult, CAPTIONBUTTON_MINIMIZE);
      *aIsOverridable = false;
      return NS_OK;

    case NS_THEME_WINDOW_BUTTON_CLOSE:
      aResult->width = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_CLOSE].cx;
      aResult->height = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_CLOSE].cy;
      if (!IsVistaOrLater()) {
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
      aResult->height += GetSystemMetrics(SM_CXPADDEDBORDER);
      *aIsOverridable = false;
      return NS_OK;

    case NS_THEME_WINDOW_BUTTON_BOX:
    case NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED:
      if (nsUXThemeData::CheckForCompositor()) {
        aResult->width = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_BUTTONBOX].cx;
        aResult->height = nsUXThemeData::sCommandButtons[CMDBUTTONIDX_BUTTONBOX].cy
                          - GetSystemMetrics(SM_CYFRAME)
                          - GetSystemMetrics(SM_CXPADDEDBORDER);
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

  int32_t part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  HDC hdc = gfxWindowsPlatform::GetPlatform()->GetScreenDC();
  if (!hdc)
    return NS_ERROR_FAILURE;

  SIZE sz;
  GetThemePartSize(theme, hdc, part, state, nullptr, sizeReq, &sz);
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
nsNativeThemeWin::WidgetStateChanged(nsIFrame* aFrame, uint8_t aWidgetType, 
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

  
  if (!IsVistaOrLater() &&
      (aWidgetType == NS_THEME_SCROLLBAR_TRACK_VERTICAL || 
      aWidgetType == NS_THEME_SCROLLBAR_TRACK_HORIZONTAL)) {
    *aShouldRepaint = false;
    return NS_OK;
  }

  
  
  if (IsVistaOrLater() &&
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
                                      uint8_t aWidgetType)
{
  
  

  if (aPresContext && !aPresContext->PresShell()->IsThemeSupportEnabled())
    return false;

  HANDLE theme = nullptr;
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
nsNativeThemeWin::WidgetIsContainer(uint8_t aWidgetType)
{
  
  if (aWidgetType == NS_THEME_DROPDOWN_BUTTON || 
      aWidgetType == NS_THEME_RADIO ||
      aWidgetType == NS_THEME_CHECKBOX)
    return false;
  return true;
}

bool
nsNativeThemeWin::ThemeDrawsFocusForWidget(uint8_t aWidgetType)
{
  return false;
}

bool
nsNativeThemeWin::ThemeNeedsComboboxDropmarker()
{
  return true;
}

bool
nsNativeThemeWin::WidgetAppearanceDependsOnWindowFocus(uint8_t aWidgetType)
{
  switch (aWidgetType) {
    case NS_THEME_WINDOW_TITLEBAR:
    case NS_THEME_WINDOW_TITLEBAR_MAXIMIZED:
    case NS_THEME_WINDOW_FRAME_LEFT:
    case NS_THEME_WINDOW_FRAME_RIGHT:
    case NS_THEME_WINDOW_FRAME_BOTTOM:
    case NS_THEME_WINDOW_BUTTON_CLOSE:
    case NS_THEME_WINDOW_BUTTON_MINIMIZE:
    case NS_THEME_WINDOW_BUTTON_MAXIMIZE:
    case NS_THEME_WINDOW_BUTTON_RESTORE:
      return true;
    default:
      return false;
  }
}

bool
nsNativeThemeWin::ShouldHideScrollbars()
{
  return WinUtils::ShouldHideScrollbars();
}

nsITheme::Transparency
nsNativeThemeWin::GetWidgetTransparency(nsIFrame* aFrame, uint8_t aWidgetType)
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
  case NS_THEME_PROGRESSBAR:
  case NS_THEME_PROGRESSBAR_VERTICAL:
  case NS_THEME_PROGRESSBAR_CHUNK:
  case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
  case NS_THEME_RANGE:
    return eTransparent;
  }

  HANDLE theme = GetTheme(aWidgetType);
  
  if (!theme) {
    
    if (aWidgetType == NS_THEME_MENUPOPUP || aWidgetType == NS_THEME_TOOLTIP) {
      return eOpaque;
    }
    return eUnknownTransparency;
  }

  int32_t part, state;
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
                                      uint8_t aWidgetType)
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
    case NS_THEME_NUMBER_INPUT:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO:
    case NS_THEME_RANGE:
    case NS_THEME_RANGE_THUMB:
    case NS_THEME_GROUPBOX:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_NON_DISAPPEARING:
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
                                  uint8_t aWidgetType,
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
    case NS_THEME_NUMBER_INPUT:
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
                                   uint8_t aWidgetType,
                                   nsIntMargin* aResult)
{
  switch (aWidgetType) {
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM: {
      int32_t part, state;
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
                                       uint8_t aWidgetType,
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
    case NS_THEME_SCROLLBAR_NON_DISAPPEARING:
    {
      aResult->SizeTo(::GetSystemMetrics(SM_CXHSCROLL),
                      ::GetSystemMetrics(SM_CYVSCROLL));
      break;
    }
    case NS_THEME_RANGE_THUMB: {
      if (IsRangeHorizontal(aFrame)) {
        (*aResult).width = 12;
        (*aResult).height = 20;
      } else {
        (*aResult).width = 20;
        (*aResult).height = 12;
      }
      *aIsOverridable = false;
      break;
    }
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
    case NS_THEME_NUMBER_INPUT:
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


nsresult nsNativeThemeWin::ClassicGetThemePartAndState(nsIFrame* aFrame, uint8_t aWidgetType,
                                 int32_t& aPart, int32_t& aState, bool& aFocused)
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
          const nsStyleUserInterface *uiData = aFrame->StyleUserInterface();
          
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
    case NS_THEME_NUMBER_INPUT:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_RANGE:
    case NS_THEME_RANGE_THUMB:
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



static void DrawTab(HDC hdc, const RECT& R, int32_t aPosition, bool aSelected,
                    bool aDrawLeft, bool aDrawRight)
{
  int32_t leftFlag, topFlag, rightFlag, lightFlag, shadeFlag;  
  RECT topRect, sideRect, bottomRect, lightRect, shadeRect;
  int32_t selectedOffset, lOffset, rOffset;

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

static void DrawMenuImage(HDC hdc, const RECT& rc, int32_t aComponent, uint32_t aColor)
{
  
  
  
  HDC hMemoryDC = ::CreateCompatibleDC(hdc);
  if (hMemoryDC) {
    
    
    int checkW = ::GetSystemMetrics(SM_CXMENUCHECK);
    int checkH = ::GetSystemMetrics(SM_CYMENUCHECK);

    HBITMAP hMonoBitmap = ::CreateBitmap(checkW, checkH, 1, 1, nullptr);
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

void nsNativeThemeWin::DrawCheckedRect(HDC hdc, const RECT& rc, int32_t fore, int32_t back,
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
      ::SetBrushOrgEx(hdc, vpOrg.x + rc.left, vpOrg.y + rc.top, nullptr);
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
                                  uint8_t aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aDirtyRect)
{
  int32_t part, state;
  bool focused;
  nsresult rv;
  rv = ClassicGetThemePartAndState(aFrame, aWidgetType, part, state, focused);
  if (NS_FAILED(rv))
    return rv;

  if (AssumeThemePartAndStateAreTransparent(part, state)) {
    return NS_OK;
  }

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
      int32_t oldTA;
      
      oldTA = ::SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
      ::DrawFrameControl(hdc, &widgetRect, part, state);
      ::SetTextAlign(hdc, oldTA);

      
      
      if (focused && (aWidgetType == NS_THEME_CHECKBOX || aWidgetType == NS_THEME_RADIO)) {
        
        POINT vpOrg;
        ::GetViewportOrgEx(hdc, &vpOrg);
        ::SetBrushOrgEx(hdc, vpOrg.x + widgetRect.left, vpOrg.y + widgetRect.top, nullptr);
        int32_t oldColor;
        oldColor = ::SetTextColor(hdc, 0);
        
        ::DrawFocusRect(hdc, &widgetRect);
        ::SetTextColor(hdc, oldColor);
      }
      break;
    }
    
    case NS_THEME_NUMBER_INPUT:
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
    case NS_THEME_RANGE_THUMB:
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
    
    case NS_THEME_RANGE:
    case NS_THEME_SCALE_VERTICAL:
    case NS_THEME_SCALE_HORIZONTAL: { 
      const int32_t trackWidth = 4;
      
      
      if (aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
          (aWidgetType == NS_THEME_RANGE && IsRangeHorizontal(aFrame))) {
        widgetRect.top += (widgetRect.bottom - widgetRect.top - trackWidth) / 2;
        widgetRect.bottom = widgetRect.top + trackWidth;
      }
      else {
        if (!IsFrameRTL(aFrame)) {
          widgetRect.left += (widgetRect.right - widgetRect.left - trackWidth) / 2;
          widgetRect.right = widgetRect.left + trackWidth;
        } else {
          widgetRect.right -= (widgetRect.right - widgetRect.left - trackWidth) / 2;
          widgetRect.left = widgetRect.right - trackWidth;
        }
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

      bool indeterminate = IsIndeterminateProgress(stateFrame, eventStates);
      bool vertical = IsVerticalProgress(stateFrame) ||
                      aWidgetType == NS_THEME_PROGRESSBAR_CHUNK_VERTICAL;
      int32_t overlayPart = GetProgressOverlayStyle(vertical);

      nsIContent* content = aFrame->GetContent();
      if (!indeterminate || !content) {
        ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_HIGHLIGHT+1));
        break;
      }

      RECT overlayRect =
        CalculateProgressOverlayRect(aFrame, &widgetRect, vertical,
                                     indeterminate, true);

      ::FillRect(hdc, &overlayRect, (HBRUSH) (COLOR_HIGHLIGHT+1));

      if (!QueueAnimatedContentForRefresh(aFrame->GetContent(), 30)) {
        NS_WARNING("unable to animate progress widget!");
      }
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
      uint32_t color = COLOR_MENUTEXT;
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
      int32_t offset = GetSystemMetrics(SM_CXFRAME);
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
      int32_t oldTA = SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
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

uint32_t
nsNativeThemeWin::GetWidgetNativeDrawingFlags(uint8_t aWidgetType)
{
  switch (aWidgetType) {
    case NS_THEME_BUTTON:
    case NS_THEME_NUMBER_INPUT:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_TEXTFIELD_MULTILINE:

    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXTFIELD:
      return
        gfxWindowsNativeDrawing::CANNOT_DRAW_TO_COLOR_ALPHA |
        gfxWindowsNativeDrawing::CAN_AXIS_ALIGNED_SCALE |
        gfxWindowsNativeDrawing::CANNOT_COMPLEX_TRANSFORM;

    
    case NS_THEME_RANGE:
    case NS_THEME_RANGE_THUMB:
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
