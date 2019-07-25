







































#include <windows.h>
#include "nsNativeThemeWin.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsTransform2D.h"
#include "nsThemeConstants.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIEventStateManager.h"
#include "nsINameSpaceManager.h"
#include "nsILookAndFeel.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIMenuFrame.h"
#include "nsWidgetAtoms.h"
#include <malloc.h>
#include "nsWindow.h"
#include "nsIComboboxControlFrame.h"

#include "gfxPlatform.h"
#include "gfxContext.h"
#include "gfxMatrix.h"
#include "gfxWindowsSurface.h"
#include "gfxWindowsNativeDrawing.h"

#include "nsUXThemeData.h"
#include "nsUXThemeConstants.h"

NS_IMPL_ISUPPORTS1(nsNativeThemeWin, nsITheme)

#ifdef WINCE





#define FrameRect moz_FrameRect
#define GetViewportOrgEx moz_GetViewportOrgEx

static int FrameRect(HDC inDC, CONST RECT *inRect, HBRUSH inBrush)
 {
   HBRUSH oldBrush = (HBRUSH)SelectObject(inDC, inBrush);
   RECT myRect = *inRect;
   InflateRect(&myRect, 1, 1); 

   
   

   
   MoveToEx(inDC, myRect.left, myRect.top, (LPPOINT) NULL);
   
   LineTo(inDC, myRect.right, myRect.top);
   
   LineTo(inDC, myRect.right, myRect.bottom);
   
   LineTo(inDC, myRect.left, myRect.bottom);
   
   LineTo(inDC, myRect.left, myRect.top);

   SelectObject(inDC, oldBrush);
   return 1;
}

static BOOL
GetViewportOrgEx(HDC hdc, LPPOINT lpPoint)
{
  SetViewportOrgEx(hdc, 0, 0, lpPoint);
  if (lpPoint->x != 0 || lpPoint->y != 0)
    SetViewportOrgEx(hdc, lpPoint->x, lpPoint->y, NULL);
  return TRUE;
}
#endif

static inline bool IsHTMLContent(nsIFrame *frame)
{
  nsIContent* content = frame->GetContent();
  return content && content->IsHTML();
}

nsNativeThemeWin::nsNativeThemeWin() {
  
  
  
}

nsNativeThemeWin::~nsNativeThemeWin() {
  nsUXThemeData::Invalidate();
}

static void GetNativeRect(const nsIntRect& aSrc, RECT& aDst)
{
  aDst.top = aSrc.y;
  aDst.bottom = aSrc.y + aSrc.height;
  aDst.left = aSrc.x;
  aDst.right = aSrc.x + aSrc.width;
}

static PRBool IsTopLevelMenu(nsIFrame *aFrame)
{
  PRBool isTopLevel(PR_FALSE);
  nsIMenuFrame *menuFrame = do_QueryFrame(aFrame);
  if (menuFrame) {
    isTopLevel = menuFrame->IsOnMenuBar();
  }
  return isTopLevel;
}

static MARGINS GetCheckboxMargins(HANDLE theme, HDC hdc)
{
    MARGINS checkboxContent = {0};
    nsUXThemeData::getThemeMargins(theme, hdc, MENU_POPUPCHECK, MCB_NORMAL, TMT_CONTENTMARGINS, NULL, &checkboxContent);
    return checkboxContent;
}
static SIZE GetCheckboxBGSize(HANDLE theme, HDC hdc)
{
    SIZE checkboxSize;
    nsUXThemeData::getThemePartSize(theme, hdc, MENU_POPUPCHECK, MC_CHECKMARKNORMAL, NULL, TS_TRUE, &checkboxSize);

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
    nsUXThemeData::getThemeMargins(theme, hdc, MENU_POPUPCHECKBACKGROUND, MCB_NORMAL, TMT_SIZINGMARGINS, NULL, &checkboxBGSizing);
    nsUXThemeData::getThemeMargins(theme, hdc, MENU_POPUPCHECKBACKGROUND, MCB_NORMAL, TMT_CONTENTMARGINS, NULL, &checkboxBGContent);

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
    nsUXThemeData::getThemePartSize(theme, hdc, MENU_POPUPGUTTER, 0, NULL, TS_TRUE, &gutterSize);

    SIZE checkboxBGSize(GetCheckboxBGBounds(theme, hdc));

    SIZE itemSize;
    nsUXThemeData::getThemePartSize(theme, hdc, MENU_POPUPITEM, MPI_NORMAL, NULL, TS_TRUE, &itemSize);

    int width = PR_MAX(itemSize.cx, checkboxBGSize.cx + gutterSize.cx);
    int height = PR_MAX(itemSize.cy, checkboxBGSize.cy);
    SIZE ret;
    ret.cx = width;
    ret.cy = height;
    return ret;
}

static HRESULT DrawThemeBGRTLAware(HANDLE theme, HDC hdc, int part, int state,
                                   const RECT *widgetRect, const RECT *clipRect,
                                   PRBool isRTL)
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
      HRESULT hr = nsUXThemeData::drawThemeBG(theme, hdc, part, state, &newWRect, newCRectPtr);
      SetLayout(hdc, 0);

      if (hr == S_OK)
        return hr;
    }
  }

  
  return nsUXThemeData::drawThemeBG(theme, hdc, part, state, widgetRect, clipRect);
}

HANDLE
nsNativeThemeWin::GetTheme(PRUint8 aWidgetType)
{ 
  if (!nsUXThemeData::sIsVistaOrLater) {
    
    
    
    
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
      
      return nsUXThemeData::sIsVistaOrLater ? nsUXThemeData::GetTheme(eUXTooltip) : NULL;
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
  }
  return NULL;
}

PRInt32
nsNativeThemeWin::StandardGetState(nsIFrame* aFrame, PRUint8 aWidgetType,
                                   PRBool wantFocused)
{
  PRInt32 eventState = GetContentState(aFrame, aWidgetType);
  if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
    return TS_ACTIVE;
  if (wantFocused && eventState & NS_EVENT_STATE_FOCUS)
    return TS_FOCUSED;
  if (eventState & NS_EVENT_STATE_HOVER)
    return TS_HOVER;

  return TS_NORMAL;
}

PRBool
nsNativeThemeWin::IsMenuActive(nsIFrame *aFrame, PRUint8 aWidgetType)
{
  nsIContent* content = aFrame->GetContent();
  if (content->IsXUL() &&
      content->NodeInfo()->Equals(nsWidgetAtoms::richlistitem))
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::selected);

  return CheckBooleanAttr(aFrame, nsWidgetAtoms::mozmenuactive);
}

nsresult 
nsNativeThemeWin::GetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                       PRInt32& aPart, PRInt32& aState)
{
  if (!nsUXThemeData::sIsVistaOrLater) {
    
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

      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      } else if (IsOpenButton(aFrame) ||
                 IsCheckedButton(aFrame)) {
        aState = TS_ACTIVE;
        return NS_OK;
      }

      aState = StandardGetState(aFrame, aWidgetType, PR_TRUE);
      
      
      
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
      PRBool isXULCheckboxRadio = PR_FALSE;

      if (!aFrame) {
        aState = TS_NORMAL;
      } else {
        if (GetCheckedOrSelected(aFrame, !isCheckbox)) {
          inputState = CHECKED;
        } if (isCheckbox && GetIndeterminate(aFrame)) {
          inputState = INDETERMINATE;
        }

        if (IsDisabled(isXULCheckboxRadio ? aFrame->GetParent() : aFrame)) {
          aState = TS_DISABLED;
        } else {
          aState = StandardGetState(aFrame, aWidgetType, PR_FALSE);
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
      if (nsUXThemeData::sIsVistaOrLater) {
        







        aPart = TFP_EDITBORDER_NOSCROLL;

        if (!aFrame) {
          aState = TFS_EDITBORDER_NORMAL;
        } else if (IsDisabled(aFrame)) {
          aState = TFS_EDITBORDER_DISABLED;
        } else if (IsReadOnly(aFrame)) {
          
          aState = TFS_EDITBORDER_NORMAL;
        } else {
          PRInt32 eventState = GetContentState(aFrame, aWidgetType);
          nsIContent* content = aFrame->GetContent();

          


          if (content && content->IsXUL() && IsFocused(aFrame))
            aState = TFS_EDITBORDER_FOCUSED;
          else if (eventState & NS_EVENT_STATE_ACTIVE || eventState & NS_EVENT_STATE_FOCUS)
            aState = TFS_EDITBORDER_FOCUSED;
          else if (eventState & NS_EVENT_STATE_HOVER)
            aState = TFS_EDITBORDER_HOVER;
          else
            aState = TFS_EDITBORDER_NORMAL;
        }
      } else {
        aPart = TFP_TEXTFIELD;
        
        if (!aFrame)
          aState = TS_NORMAL;
        else if (IsDisabled(aFrame))
          aState = TS_DISABLED;
        else if (IsReadOnly(aFrame))
          aState = TFS_READONLY;
        else
          aState = StandardGetState(aFrame, aWidgetType, PR_TRUE);
      }

      return NS_OK;
    }
    case NS_THEME_TOOLTIP: {
      aPart = TTP_STANDARD;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR: {
      aPart = PP_BAR;
      aState = TS_NORMAL;
      return NS_OK;
    }
    case NS_THEME_PROGRESSBAR_CHUNK: {
      aPart = PP_CHUNK;
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

      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      }
      if (IsOpenButton(aFrame)) {
        aState = TS_ACTIVE;
        return NS_OK;
      }
      PRInt32 eventState = GetContentState(aFrame, aWidgetType);
      if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
        aState = TS_ACTIVE;
      else if (eventState & NS_EVENT_STATE_HOVER) {
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
      if (!aFrame)
        aState += TS_NORMAL;
      else if (IsDisabled(aFrame))
        aState += TS_DISABLED;
      else {
        PRInt32 eventState = GetContentState(aFrame, aWidgetType);
        nsIFrame *parent = aFrame->GetParent();
        PRInt32 parentState = GetContentState(parent, parent->GetStyleDisplay()->mAppearance);
        if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
          aState += TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_HOVER)
          aState += TS_HOVER;
        else if (nsUXThemeData::sIsVistaOrLater && parentState & NS_EVENT_STATE_HOVER)
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
      if (!aFrame)
        aState = TS_NORMAL;
      else if (IsDisabled(aFrame))
        aState = TS_DISABLED;
      else {
        PRInt32 eventState = GetContentState(aFrame, aWidgetType);
        if (eventState & NS_EVENT_STATE_ACTIVE) 
                                                
                                                
          aState = TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_HOVER)
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
      if (!aFrame)
        aState = TS_NORMAL;
      else if (IsDisabled(aFrame)) {
        aState = TKP_DISABLED;
      }
      else {
        PRInt32 eventState = GetContentState(aFrame, aWidgetType);
        if (eventState & NS_EVENT_STATE_ACTIVE) 
                                                
                                                
          aState = TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_FOCUS)
          aState = TKP_FOCUSED;
        else if (eventState & NS_EVENT_STATE_HOVER)
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
      if (!aFrame)
        aState = TS_NORMAL;
      else if (IsDisabled(aFrame))
        aState = TS_DISABLED;
      else
        aState = StandardGetState(aFrame, aWidgetType, PR_FALSE);
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
      if (nsUXThemeData::sIsVistaOrLater) {
        
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
        
        if (parent && parent->GetChildAt(0) == content) {
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
      
      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      }

      if (IsSelectedTab(aFrame)) {
        aPart = TABP_TAB_SELECTED;
        aState = TS_ACTIVE; 
      }
      else
        aState = StandardGetState(aFrame, aWidgetType, PR_TRUE);
      
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
      
      aState = StandardGetState(aFrame, aWidgetType, PR_TRUE);
      
      return NS_OK;
    }
    case NS_THEME_DROPDOWN: {
      nsIContent* content = aFrame->GetContent();
      PRBool isHTML = content && content->IsHTML();

      


      if (isHTML)
        aPart = CBP_DROPBORDER;
      else
        aPart = CBP_DROPFRAME;

      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
      } else if (IsReadOnly(aFrame)) {
        aState = TS_NORMAL;
      } else if (IsOpenButton(aFrame)) {
        aState = TS_ACTIVE;
      } else {
        PRInt32 eventState = GetContentState(aFrame, aWidgetType);
        if (isHTML && eventState & NS_EVENT_STATE_FOCUS)
          aState = TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_HOVER && eventState & NS_EVENT_STATE_ACTIVE)
          aState = TS_ACTIVE;
        else if (eventState & NS_EVENT_STATE_HOVER)
          aState = TS_HOVER;
        else
          aState = TS_NORMAL;
      }

      return NS_OK;
    }
    case NS_THEME_DROPDOWN_BUTTON: {
      PRBool isHTML = IsHTMLContent(aFrame);
      nsIFrame* parentFrame = aFrame->GetParent();
      PRBool isMenulist = !isHTML && parentFrame->GetType() == nsWidgetAtoms::menuFrame;
      PRBool isOpen = PR_FALSE;

      
      if (isHTML || isMenulist)
        aFrame = parentFrame;

      aPart = nsUXThemeData::sIsVistaOrLater ? CBP_DROPMARKER_VISTA : CBP_DROPMARKER;

      
      
      
      if (isHTML && IsWidgetStyled(aFrame->PresContext(), aFrame, NS_THEME_DROPDOWN))
        aPart = CBP_DROPMARKER;

      if (IsDisabled(aFrame)) {
        aState = TS_DISABLED;
        return NS_OK;
      }

      if (isHTML) {
        nsIComboboxControlFrame* ccf = do_QueryFrame(aFrame);
        isOpen = (ccf && ccf->IsDroppedDown());
      }
      else
        isOpen = IsOpenButton(aFrame);

      if (nsUXThemeData::sIsVistaOrLater) {
        if (isHTML) {
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
      PRInt32 eventState = GetContentState(aFrame, aWidgetType);

      
      if (eventState & NS_EVENT_STATE_ACTIVE) {
        if (isOpen && (isHTML || isMenulist)) {
          
          
          return NS_OK;
        }
        aState = TS_ACTIVE;
      }
      else if (eventState & NS_EVENT_STATE_HOVER) {
        
        
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
      PRBool isTopLevel = PR_FALSE;
      PRBool isOpen = PR_FALSE;
      PRBool isHover = PR_FALSE;
      nsIMenuFrame *menuFrame = do_QueryFrame(aFrame);

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

        
        if (IsDisabled(aFrame))
          aState += 3;
      } else {
        aPart = MENU_POPUPITEM;

        if (isHover)
          aState = MPI_HOT;
        else
          aState = MPI_NORMAL;

        
        if (IsDisabled(aFrame))
          aState += 2;
      }

      return NS_OK;
    }
    case NS_THEME_MENUSEPARATOR:
      aPart = MENU_POPUPSEPARATOR;
      aState = 0;
      return NS_OK;
    case NS_THEME_MENUARROW:
      aPart = MENU_POPUPSUBMENU;
      aState = IsDisabled(aFrame) ? MSM_DISABLED : MSM_NORMAL;
      return NS_OK;
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
      {
        PRBool isChecked;
        PRBool isDisabled;

        isChecked = CheckBooleanAttr(aFrame, nsWidgetAtoms::checked);
        isDisabled = CheckBooleanAttr(aFrame, nsWidgetAtoms::disabled);

        aPart = MENU_POPUPCHECK;
        aState = MC_CHECKMARKNORMAL;

        
        if (aWidgetType == NS_THEME_MENURADIO)
          aState += 2;

        
        if (isDisabled)
          aState += 1;

        return NS_OK;
      }
    case NS_THEME_MENUITEMTEXT:
    case NS_THEME_MENUIMAGE:
      aPart = -1;
      aState = 0;
      return NS_OK;
  }

  aPart = 0;
  aState = 0;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNativeThemeWin::DrawWidgetBackground(nsIRenderingContext* aContext,
                                       nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       const nsRect& aRect,
                                       const nsRect& aDirtyRect)
{
  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return ClassicDrawWidgetBackground(aContext, aFrame, aWidgetType, aRect, aDirtyRect); 

  if (!nsUXThemeData::drawThemeBG)
    return NS_ERROR_FAILURE;    

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDeviceContext> dc;
  aContext->GetDeviceContext(*getter_AddRefs(dc));
  gfxFloat p2a = gfxFloat(dc->AppUnitsPerDevPixel());
  RECT widgetRect;
  RECT clipRect;
  gfxRect tr(aRect.x, aRect.y, aRect.width, aRect.height),
          dr(aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);

  tr.ScaleInverse(p2a);
  dr.ScaleInverse(p2a);

  
  if (aWidgetType == NS_THEME_DROPDOWN_BUTTON &&
      part == CBP_DROPMARKER_VISTA && IsHTMLContent(aFrame))
  {
    tr.pos.y -= 1.0;
    tr.size.width += 1.0;
    tr.size.height += 2.0;

    dr.pos.y -= 1.0;
    dr.size.width += 1.0;
    dr.size.height += 2.0;
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
    fprintf (stderr, "xform: %f %f %f %f [%f %f]\n", m.xx, m.yx, m.xy, m.yy, m.x0, m.y0);
    fprintf (stderr, "tr: [%d %d %d %d]\ndr: [%d %d %d %d]\noff: [%f %f]\n",
             tr.x, tr.y, tr.width, tr.height, dr.x, dr.y, dr.width, dr.height,
             offset.x, offset.y);
  }
#endif

  
  
  if (aWidgetType == NS_THEME_TAB) {
    PRBool isLeft = IsLeftToSelectedTab(aFrame);
    PRBool isRight = !isLeft && IsRightToSelectedTab(aFrame);

    if (isLeft || isRight) {
      
      
      
      PRInt32 edgeSize = 2;
    
      
      
      
      if (isLeft)
        
        widgetRect.right += edgeSize;
      else
        
        widgetRect.left -= edgeSize;
    }
  }

  
  
  
  if (aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
      aWidgetType == NS_THEME_SCALE_VERTICAL) {
    RECT contentRect;
    nsUXThemeData::getThemeContentRect(theme, hdc, part, state, &widgetRect, &contentRect);

    SIZE siz;
    nsUXThemeData::getThemePartSize(theme, hdc, part, state, &widgetRect, 1, &siz);

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

    nsUXThemeData::drawThemeBG(theme, hdc, part, state, &contentRect, &clipRect);
  }
  else if (aWidgetType == NS_THEME_MENUCHECKBOX || aWidgetType == NS_THEME_MENURADIO)
  {
      PRBool isChecked = PR_FALSE;
      isChecked = CheckBooleanAttr(aFrame, nsWidgetAtoms::checked);

      if (isChecked)
      {
        int bgState = MCB_NORMAL;
        PRBool isDisabled = IsDisabled(aFrame);

        
        if (isDisabled)
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

        nsUXThemeData::drawThemeBG(theme, hdc, MENU_POPUPCHECKBACKGROUND, bgState, &checkBGRect, &clipRect);

        MARGINS checkMargins = GetCheckboxMargins(theme, hdc);
        RECT checkRect = checkBGRect;
        checkRect.left += checkMargins.cxLeftWidth;
        checkRect.right -= checkMargins.cxRightWidth;
        checkRect.top += checkMargins.cyTopHeight;
        checkRect.bottom -= checkMargins.cyBottomHeight;
        nsUXThemeData::drawThemeBG(theme, hdc, MENU_POPUPCHECK, state, &checkRect, &clipRect);
      }
  }
  else if (aWidgetType == NS_THEME_MENUPOPUP)
  {
    nsUXThemeData::drawThemeBG(theme, hdc, MENU_POPUPBORDERS,  0, &widgetRect, &clipRect);
    SIZE borderSize;
    nsUXThemeData::getThemePartSize(theme, hdc, MENU_POPUPBORDERS, 0, NULL, TS_TRUE, &borderSize);

    RECT bgRect = widgetRect;
    bgRect.top += borderSize.cy;
    bgRect.bottom -= borderSize.cy;
    bgRect.left += borderSize.cx;
    bgRect.right -= borderSize.cx;

    nsUXThemeData::drawThemeBG(theme, hdc, MENU_POPUPBACKGROUND,  0, &bgRect, &clipRect);

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

    nsUXThemeData::drawThemeBG(theme, hdc, MENU_POPUPSEPARATOR,  0, &sepRect, &clipRect);
  }
  
  else if (aWidgetType == NS_THEME_MENUARROW ||
           aWidgetType == NS_THEME_RESIZER)
  {
    DrawThemeBGRTLAware(theme, hdc, part, state,
                        &widgetRect, &clipRect, IsFrameRTL(aFrame));
  }
  
  
  else if (part >= 0) {
    nsUXThemeData::drawThemeBG(theme, hdc, part, state, &widgetRect, &clipRect);
  }

  
  
  if ((aWidgetType == NS_THEME_CHECKBOX || aWidgetType == NS_THEME_RADIO) &&
      aFrame->GetContent()->IsHTML() ||
      aWidgetType == NS_THEME_SCALE_HORIZONTAL ||
      aWidgetType == NS_THEME_SCALE_VERTICAL) {
      PRInt32 contentState;
      contentState = GetContentState(aFrame, aWidgetType);  

      if (contentState & NS_EVENT_STATE_FOCUS) {
        POINT vpOrg;
        HPEN hPen = nsnull;

        PRUint8 id = SaveDC(hdc);

        ::SelectClipRgn(hdc, NULL);
        ::GetViewportOrgEx(hdc, &vpOrg);
        ::SetBrushOrgEx(hdc, vpOrg.x + widgetRect.left, vpOrg.y + widgetRect.top, NULL);

#ifndef WINCE
        
        
        if (nsUXThemeData::sIsVistaOrLater && aWidgetType == NS_THEME_CHECKBOX) {
          LOGBRUSH lb;
          lb.lbStyle = BS_SOLID;
          lb.lbColor = RGB(255,255,255);
          lb.lbHatch = 0;

          hPen = ::ExtCreatePen(PS_COSMETIC|PS_ALTERNATE, 1, &lb, 0, NULL);
          ::SelectObject(hdc, hPen);

          
          if (contentState & NS_EVENT_STATE_ACTIVE) {
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
#else
        ::SetTextColor(hdc, 0);
        ::DrawFocusRect(hdc, &widgetRect);
#endif
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
    nsUXThemeData::drawThemeEdge(theme, hdc, RP_BAND, 0, &widgetRect, EDGE_ETCHED, BF_TOP, NULL);
  }
  else if (aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL ||
           aWidgetType == NS_THEME_SCROLLBAR_THUMB_VERTICAL)
  {
    

    SIZE gripSize;
    MARGINS thumbMgns;
    int gripPart = (aWidgetType == NS_THEME_SCROLLBAR_THUMB_HORIZONTAL) ?
                   SP_GRIPPERHOR : SP_GRIPPERVERT;

    if (nsUXThemeData::getThemePartSize(theme, hdc, gripPart, state, NULL, TS_TRUE, &gripSize) == S_OK &&
        nsUXThemeData::getThemeMargins(theme, hdc, part, state, TMT_CONTENTMARGINS, NULL, &thumbMgns) == S_OK &&
        gripSize.cx + thumbMgns.cxLeftWidth + thumbMgns.cxRightWidth <= widgetRect.right - widgetRect.left &&
        gripSize.cy + thumbMgns.cyTopHeight + thumbMgns.cyBottomHeight <= widgetRect.bottom - widgetRect.top)
    {
      nsUXThemeData::drawThemeBG(theme, hdc, gripPart, state, &widgetRect, &clipRect);
    }
  }

  nativeDrawing.EndNativeDrawing();

  if (nativeDrawing.ShouldRenderAgain())
    goto RENDER_AGAIN;

  nativeDrawing.PaintToContext();

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::GetWidgetBorder(nsIDeviceContext* aContext, 
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
      aWidgetType == NS_THEME_TOOLBAR_SEPARATOR)
    return NS_OK; 

  if (!nsUXThemeData::getThemeContentRect)
    return NS_ERROR_FAILURE;

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
  HRESULT res = nsUXThemeData::getThemeContentRect(theme, NULL, part, state, &outerRect, &contentRect);
  
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

PRBool
nsNativeThemeWin::GetWidgetPadding(nsIDeviceContext* aContext, 
                                   nsIFrame* aFrame,
                                   PRUint8 aWidgetType,
                                   nsIntMargin* aResult)
{
  switch (aWidgetType) {
    
    
    
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO:
      aResult->SizeTo(0, 0, 0, 0);
      return PR_TRUE;
  }

  HANDLE theme = GetTheme(aWidgetType);
  if (!theme)
    return PR_FALSE;

  if (aWidgetType == NS_THEME_MENUPOPUP)
  {
    SIZE popupSize;
    nsUXThemeData::getThemePartSize(theme, NULL, MENU_POPUPBORDERS,  0, NULL, TS_TRUE, &popupSize);
    aResult->top = aResult->bottom = popupSize.cy;
    aResult->left = aResult->right = popupSize.cx;
    return PR_TRUE;
  }

  if (nsUXThemeData::sIsVistaOrLater) {
    if (aWidgetType == NS_THEME_TEXTFIELD ||
        aWidgetType == NS_THEME_TEXTFIELD_MULTILINE ||
        aWidgetType == NS_THEME_DROPDOWN)
    {
      
      if (aFrame->PresContext()->HasAuthorSpecifiedRules(aFrame, NS_AUTHOR_SPECIFIED_PADDING))
        return PR_FALSE;
    }

    





    if (aWidgetType == NS_THEME_TEXTFIELD || aWidgetType == NS_THEME_TEXTFIELD_MULTILINE) {
      aResult->top = aResult->bottom = 2;
      aResult->left = aResult->right = 2;
      return PR_TRUE;
    } else if (IsHTMLContent(aFrame) && aWidgetType == NS_THEME_DROPDOWN) {
      




      aResult->top = aResult->bottom = 1;
      aResult->left = aResult->right = 1;
      return PR_TRUE;
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
        return PR_FALSE;
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
  
  return PR_TRUE;
}

PRBool
nsNativeThemeWin::GetWidgetOverflow(nsIDeviceContext* aContext, 
                                    nsIFrame* aFrame,
                                    PRUint8 aOverflowRect,
                                    nsRect* aResult)
{
  






#if 0
  if (nsUXThemeData::sIsVistaOrLater) {
    



    if (aWidgetType == NS_THEME_DROPDOWN_BUTTON &&
        IsHTMLContent(aFrame) &&
        !IsWidgetStyled(aFrame->GetParent()->PresContext(),
                        aFrame->GetParent(),
                        NS_THEME_DROPDOWN))
    {
      PRInt32 p2a = aContext->AppUnitsPerDevPixel();
      
      nsMargin m(0, p2a, p2a, p2a);
      aOverflowRect->Inflate (m);
      return PR_TRUE;
    }
  }
#endif

  return PR_FALSE;
}

NS_IMETHODIMP
nsNativeThemeWin::GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       nsIntSize* aResult, PRBool* aIsOverridable)
{
  (*aResult).width = (*aResult).height = 0;
  *aIsOverridable = PR_TRUE;

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
      return NS_OK; 
  }

  if (aWidgetType == NS_THEME_MENUITEM && IsTopLevelMenu(aFrame))
      return NS_OK; 

  if (!nsUXThemeData::getThemePartSize)
    return NS_ERROR_FAILURE;
  
  
  
  
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
        *aIsOverridable = PR_FALSE;
      }
    case NS_THEME_MENUITEMTEXT:
      return NS_OK;
    case NS_THEME_MENUARROW:
      aResult->width = 26;
      aResult->height = 16;
      return NS_OK;
  }

  if (aWidgetType == NS_THEME_SCALE_THUMB_HORIZONTAL ||
      aWidgetType == NS_THEME_SCALE_THUMB_VERTICAL) {
    *aIsOverridable = PR_FALSE;
    
    
    if (nsUXThemeData::sIsVistaOrLater) {
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
  }
  else if (aWidgetType == NS_THEME_TOOLBAR_SEPARATOR) {
    
    
    aResult->width = 6;
    return NS_OK;
  }

  PRInt32 part, state;
  nsresult rv = GetThemePartAndState(aFrame, aWidgetType, part, state);
  if (NS_FAILED(rv))
    return rv;

  HDC hdc = (HDC)aContext->GetNativeGraphicData(nsIRenderingContext::NATIVE_WINDOWS_DC);
  if (!hdc)
    return NS_ERROR_FAILURE;

  PRInt32 sizeReq = 1; 
  if (aWidgetType == NS_THEME_PROGRESSBAR ||
      aWidgetType == NS_THEME_PROGRESSBAR_VERTICAL)
    sizeReq = 0; 
                 
                 
                 

  
  
  
  
  if (aWidgetType == NS_THEME_BUTTON &&
      aFrame->GetContent()->IsHTML())
    sizeReq = 0; 

  SIZE sz;
  nsUXThemeData::getThemePartSize(theme, hdc, part, state, NULL, sizeReq, &sz);
  aResult->width = sz.cx;
  aResult->height = sz.cy;

  if (aWidgetType == NS_THEME_SPINNER_UP_BUTTON ||
      aWidgetType == NS_THEME_SPINNER_DOWN_BUTTON) {
    aResult->width++;
    aResult->height = aResult->height / 2 + 1;
  }
  else if (aWidgetType == NS_THEME_MENUSEPARATOR)
  {
    SIZE gutterSize(GetGutterSize(theme,hdc));
    aResult->width += gutterSize.cx;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                     nsIAtom* aAttribute, PRBool* aShouldRepaint)
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
      aWidgetType == NS_THEME_TOOLBAR_SEPARATOR) {
    *aShouldRepaint = PR_FALSE;
    return NS_OK;
  }

  
  if (!nsUXThemeData::sIsVistaOrLater &&
      (aWidgetType == NS_THEME_SCROLLBAR_TRACK_VERTICAL || 
      aWidgetType == NS_THEME_SCROLLBAR_TRACK_HORIZONTAL)) {
    *aShouldRepaint = PR_FALSE;
    return NS_OK;
  }

  
  
  if (nsUXThemeData::sIsVistaOrLater &&
      (aWidgetType == NS_THEME_DROPDOWN || aWidgetType == NS_THEME_DROPDOWN_BUTTON) &&
      IsHTMLContent(aFrame))
  {
    *aShouldRepaint = PR_TRUE;
    return NS_OK;
  }

  
  
  
  if (!aAttribute) {
    
    *aShouldRepaint = PR_TRUE;
  }
  else {
    
    
    *aShouldRepaint = PR_FALSE;
    if (aAttribute == nsWidgetAtoms::disabled ||
        aAttribute == nsWidgetAtoms::checked ||
        aAttribute == nsWidgetAtoms::selected ||
        aAttribute == nsWidgetAtoms::readonly ||
        aAttribute == nsWidgetAtoms::open ||
        aAttribute == nsWidgetAtoms::mozmenuactive ||
        aAttribute == nsWidgetAtoms::focused)
      *aShouldRepaint = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeWin::ThemeChanged()
{
  nsUXThemeData::Invalidate();
  return NS_OK;
}

PRBool 
nsNativeThemeWin::ThemeSupportsWidget(nsPresContext* aPresContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType)
{
  
  

  if (aPresContext && !aPresContext->PresShell()->IsThemeSupportEnabled())
    return PR_FALSE;

  HANDLE theme = NULL;
  if (aWidgetType == NS_THEME_CHECKBOX_CONTAINER)
    theme = GetTheme(NS_THEME_CHECKBOX);
  else if (aWidgetType == NS_THEME_RADIO_CONTAINER)
    theme = GetTheme(NS_THEME_RADIO);
  else
    theme = GetTheme(aWidgetType);

  if ((theme) || (!theme && ClassicThemeSupportsWidget(aPresContext, aFrame, aWidgetType)))
    
    return (!IsWidgetStyled(aPresContext, aFrame, aWidgetType));
  
  return PR_FALSE;
}

PRBool 
nsNativeThemeWin::WidgetIsContainer(PRUint8 aWidgetType)
{
  
  if (aWidgetType == NS_THEME_DROPDOWN_BUTTON || 
      aWidgetType == NS_THEME_RADIO ||
      aWidgetType == NS_THEME_CHECKBOX)
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsNativeThemeWin::ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType)
{
  return PR_FALSE;
}

PRBool
nsNativeThemeWin::ThemeNeedsComboboxDropmarker()
{
  return PR_TRUE;
}

nsTransparencyMode
nsNativeThemeWin::GetWidgetTransparency(PRUint8 aWidgetType)
{
  return eTransparencyOpaque;
}



PRBool 
nsNativeThemeWin::ClassicThemeSupportsWidget(nsPresContext* aPresContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType)
{
  switch (aWidgetType) {
    case NS_THEME_MENUBAR:
    case NS_THEME_MENUPOPUP:
      
      if (!nsUXThemeData::sFlatMenus)
        return PR_FALSE;
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
    case NS_THEME_RESIZER:
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
      return PR_TRUE;
  }
  return PR_FALSE;
}

nsresult
nsNativeThemeWin::ClassicGetWidgetBorder(nsIDeviceContext* aContext, 
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
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM: {
      PRInt32 part, state;
      PRBool focused;
      nsresult rv;

      rv = ClassicGetThemePartAndState(aFrame, aWidgetType, part, state, focused);
      if (NS_FAILED(rv))
        return rv;

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
      break;
    }
    default:
      (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right = 0;
      break;
  }
  return NS_OK;
}

nsresult
nsNativeThemeWin::ClassicGetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                       PRUint8 aWidgetType,
                                       nsIntSize* aResult, PRBool* aIsOverridable)
{
  (*aResult).width = (*aResult).height = 0;
  *aIsOverridable = PR_TRUE;
  switch (aWidgetType) {
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX:
      (*aResult).width = (*aResult).height = 13;
      break;
    case NS_THEME_MENUCHECKBOX:
    case NS_THEME_MENURADIO:
    case NS_THEME_MENUARROW:
#ifdef WINCE
      (*aResult).width =  16;
      (*aResult).height = 16;
#else
      (*aResult).width = ::GetSystemMetrics(SM_CXMENUCHECK);
      (*aResult).height = ::GetSystemMetrics(SM_CYMENUCHECK);
#endif
      break;
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
      (*aResult).width = ::GetSystemMetrics(SM_CXVSCROLL);
      (*aResult).height = ::GetSystemMetrics(SM_CYVSCROLL);
      *aIsOverridable = PR_FALSE;
      break;
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
      (*aResult).width = ::GetSystemMetrics(SM_CXHSCROLL);
      (*aResult).height = ::GetSystemMetrics(SM_CYHSCROLL);
      *aIsOverridable = PR_FALSE;
      break;
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
      
      
      

        
      break;
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
      (*aResult).width = 12;
      (*aResult).height = 20;
      *aIsOverridable = PR_FALSE;
      break;
    case NS_THEME_SCALE_THUMB_VERTICAL:
      (*aResult).width = 20;
      (*aResult).height = 12;
      *aIsOverridable = PR_FALSE;
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
#ifndef WINCE
      NONCLIENTMETRICS nc;
      nc.cbSize = sizeof(nc);
      if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(nc), &nc, 0))
        (*aResult).width = (*aResult).height = abs(nc.lfStatusFont.lfHeight) + 4;
      else
#endif
        (*aResult).width = (*aResult).height = 15;
      break;
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
#ifndef WINCE
      (*aResult).width = ::GetSystemMetrics(SM_CXVSCROLL);
      (*aResult).height = ::GetSystemMetrics(SM_CYVTHUMB);
#else
      (*aResult).width = 15;
      (*aResult).height = 15;
#endif
      
      
      if (!GetTheme(aWidgetType))
        (*aResult).height >>= 1;
      *aIsOverridable = PR_FALSE;
      break;
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
#ifndef WINCE
      (*aResult).width = ::GetSystemMetrics(SM_CXHTHUMB);
      (*aResult).height = ::GetSystemMetrics(SM_CYHSCROLL);
#else
      (*aResult).width = 15;
      (*aResult).height = 15;
#endif
      
      
      if (!GetTheme(aWidgetType))
        (*aResult).width >>= 1;
      *aIsOverridable = PR_FALSE;
      break;
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
#ifndef WINCE
      (*aResult).width = ::GetSystemMetrics(SM_CXHTHUMB) << 1;
#else
      (*aResult).width = 10;
#endif
      break;
    }
    case NS_THEME_MENUSEPARATOR:
    {
      aResult->width = 0;
      aResult->height = 10;
      break;
    }
    default:
      return NS_ERROR_FAILURE;
  }  
  return NS_OK;
}


nsresult nsNativeThemeWin::ClassicGetThemePartAndState(nsIFrame* aFrame, PRUint8 aWidgetType,
                                 PRInt32& aPart, PRInt32& aState, PRBool& aFocused)
{  
  switch (aWidgetType) {
    case NS_THEME_BUTTON: {
      PRInt32 contentState;

      aPart = DFC_BUTTON;
      aState = DFCS_BUTTONPUSH;
      aFocused = PR_FALSE;

      contentState = GetContentState(aFrame, aWidgetType);
      if (IsDisabled(aFrame))
        aState |= DFCS_INACTIVE;
      else if (IsOpenButton(aFrame))
        aState |= DFCS_PUSHED;
      else if (IsCheckedButton(aFrame))
        aState |= DFCS_CHECKED;
      else {
        if (contentState & NS_EVENT_STATE_ACTIVE && contentState & NS_EVENT_STATE_HOVER) {
          aState |= DFCS_PUSHED;
          const nsStyleUserInterface *uiData = aFrame->GetStyleUserInterface();
          
          if (uiData->mUserFocus == NS_STYLE_USER_FOCUS_NORMAL) {
#ifndef WINCE
            if (!aFrame->GetContent()->IsHTML())
              aState |= DFCS_FLAT;
#endif
            aFocused = PR_TRUE;
          }
        }
        if ((contentState & NS_EVENT_STATE_FOCUS) || 
          (aState == DFCS_BUTTONPUSH && IsDefaultButton(aFrame))) {
          aFocused = PR_TRUE;          
        }

      }

      return NS_OK;
    }
    case NS_THEME_CHECKBOX:
    case NS_THEME_RADIO: {
      PRInt32 contentState;
      aFocused = PR_FALSE;

      aPart = DFC_BUTTON;
      aState = 0;
      nsIContent* content = aFrame->GetContent();
      PRBool isCheckbox = (aWidgetType == NS_THEME_CHECKBOX);
      PRBool isChecked = GetCheckedOrSelected(aFrame, !isCheckbox);
      PRBool isIndeterminate = isCheckbox && GetIndeterminate(aFrame);

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
          (contentState & NS_EVENT_STATE_FOCUS)) {
        aFocused = PR_TRUE;
      }

      if (IsDisabled(aFrame)) {
        aState |= DFCS_INACTIVE;
      } else if (contentState & NS_EVENT_STATE_ACTIVE &&
                 contentState & NS_EVENT_STATE_HOVER) {
        aState |= DFCS_PUSHED;
      }

      return NS_OK;
    }
    case NS_THEME_MENUITEM:
    case NS_THEME_CHECKMENUITEM:
    case NS_THEME_RADIOMENUITEM: {
      PRBool isTopLevel = PR_FALSE;
      PRBool isOpen = PR_FALSE;
      PRBool isContainer = PR_FALSE;
      nsIMenuFrame *menuFrame = do_QueryFrame(aFrame);

      
      
      
      aPart = 0;
      aState = 0;

      if (menuFrame) {
        
        
        
        isTopLevel = menuFrame->IsOnMenuBar();
        isOpen = menuFrame->IsOpen();
        isContainer = menuFrame->IsMenu();
      }

      if (IsDisabled(aFrame))
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
      if (IsDisabled(aFrame))
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
      PRBool isHTML = IsHTMLContent(aFrame);
      PRBool isMenulist = !isHTML && parentFrame->GetType() == nsWidgetAtoms::menuFrame;
      PRBool isOpen = PR_FALSE;

      
      if (isHTML || isMenulist)
        aFrame = parentFrame;

      if (IsDisabled(aFrame)) {
        aState |= DFCS_INACTIVE;
        return NS_OK;
      }

#ifndef WINCE
      if (isHTML) {
        nsIComboboxControlFrame* ccf = do_QueryFrame(aFrame);
        isOpen = (ccf && ccf->IsDroppedDown());
      }
      else
        isOpen = IsOpenButton(aFrame);

      
      
      if (isOpen && (isHTML || isMenulist))
        return NS_OK;

      PRInt32 eventState = GetContentState(aFrame, aWidgetType);

      
      if (eventState & NS_EVENT_STATE_ACTIVE)
        aState |= DFCS_PUSHED | DFCS_FLAT;
#endif

      return NS_OK;
    }
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT: {
      PRInt32 contentState;

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
      
      if (IsDisabled(aFrame))
        aState |= DFCS_INACTIVE;
      else {
        contentState = GetContentState(aFrame, aWidgetType);
#ifndef WINCE
        if (contentState & NS_EVENT_STATE_HOVER && contentState & NS_EVENT_STATE_ACTIVE)
          aState |= DFCS_PUSHED | DFCS_FLAT;      
#endif
      }

      return NS_OK;
    }
    case NS_THEME_SPINNER_UP_BUTTON:
    case NS_THEME_SPINNER_DOWN_BUTTON: {
      PRInt32 contentState;

      aPart = DFC_SCROLL;
      switch (aWidgetType) {
        case NS_THEME_SPINNER_UP_BUTTON:
          aState = DFCS_SCROLLUP;
          break;
        case NS_THEME_SPINNER_DOWN_BUTTON:
          aState = DFCS_SCROLLDOWN;
          break;
      }      
      
      if (IsDisabled(aFrame))
        aState |= DFCS_INACTIVE;
      else {
        contentState = GetContentState(aFrame, aWidgetType);
        if (contentState & NS_EVENT_STATE_HOVER && contentState & NS_EVENT_STATE_ACTIVE)
          aState |= DFCS_PUSHED;
      }

      return NS_OK;    
    }
    case NS_THEME_RESIZER:    
      aPart = DFC_SCROLL;
#ifndef WINCE
      aState = (IsFrameRTL(aFrame)) ?
               DFCS_SCROLLSIZEGRIPRIGHT : DFCS_SCROLLSIZEGRIP;
#else
      aState = 0;
#endif
      return NS_OK;
    case NS_THEME_MENUSEPARATOR:
      aPart = 0;
      aState = 0;
      return NS_OK;
  }
  return NS_ERROR_FAILURE;
}



static void DrawTab(HDC hdc, const RECT& R, PRInt32 aPosition, PRBool aSelected,
                    PRBool aDrawLeft, PRBool aDrawRight)
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

#ifndef WINCE
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
#endif

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

#ifndef WINCE
      ::UnrealizeObject(brush);
#endif
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

nsresult nsNativeThemeWin::ClassicDrawWidgetBackground(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aDirtyRect)
{
  PRInt32 part, state;
  PRBool focused;
  nsresult rv;
  rv = ClassicGetThemePartAndState(aFrame, aWidgetType, part, state, focused);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDeviceContext> dc;
  aContext->GetDeviceContext(*getter_AddRefs(dc));
  gfxFloat p2a = gfxFloat(dc->AppUnitsPerDevPixel());
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

      
      if (IsDisabled(aFrame) ||
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
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
      ::DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_RECT | BF_SOFT | BF_MIDDLE | BF_ADJUST);
      if (IsDisabled(aFrame)) {
        DrawCheckedRect(hdc, widgetRect, COLOR_3DFACE, COLOR_3DHILIGHT,
                        (HBRUSH) COLOR_3DHILIGHT);
      }

      break;
    
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
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
      ::FillRect(hdc, &widgetRect, (HBRUSH) (COLOR_HIGHLIGHT+1));

      break;
    
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
#ifndef WINCE
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
#endif
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






extern PRBool gDisableNativeTheme;

NS_METHOD NS_NewNativeTheme(nsISupports *aOuter, REFNSIID aIID, void **aResult)
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
