




































#include <Gestalt.h>
#include "nsNativeThemeMac.h"
#include "nsIRenderingContext.h"
#include "nsIDrawingSurfaceMac.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsTransform2D.h"
#include "nsThemeConstants.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIAtom.h"
#include "nsIEventStateManager.h"
#include "nsINameSpaceManager.h"
#include "nsPresContext.h"
#include "nsILookAndFeel.h"
#include "nsRegionPool.h"
#include "nsGfxUtils.h"
#include "nsUnicharUtils.h"
#include "nsWidgetAtoms.h"

static PRBool sInitializedBorders = PR_FALSE;


PRBool
OnTigerOrLater()
{
  static PRBool sInitVer = PR_FALSE;
  static PRBool sOnTigerOrLater = PR_FALSE;
  if (!sInitVer) {
    long version;
    OSErr err = ::Gestalt(gestaltSystemVersion, &version);
    sOnTigerOrLater = ((err == noErr) && (version >= 0x00001040));
    sInitVer = PR_TRUE;
  }
  return sOnTigerOrLater;
}


static void 
ConvertGeckoToNativeRect(const nsRect& aSrc, Rect& aDst) 
{
  aDst.top = aSrc.y;
  aDst.bottom = aSrc.y + aSrc.height;
  aDst.left = aSrc.x;
  aDst.right = aSrc.x + aSrc.width;
}








static pascal void
DoNothing(const Rect *bounds, UInt32 eraseData, SInt16 depth, Boolean isColorDev)
{
  
}

         
NS_IMPL_ISUPPORTS1(nsNativeThemeMac, nsITheme)

nsNativeThemeMac::nsNativeThemeMac()
  : mEraseProc(nsnull)
{
  mEraseProc = NewThemeEraseUPP(DoNothing);
  if (!sInitializedBorders) {
    sInitializedBorders = PR_TRUE;
    sTextfieldBorderSize.left = sTextfieldBorderSize.top = 2;
    sTextfieldBorderSize.right = sTextfieldBorderSize.bottom = 2;
    sTextfieldBGTransparent = PR_FALSE;
    sListboxBGTransparent = PR_TRUE;
    sTextfieldDisabledBGColorID = nsILookAndFeel::eColor__moz_field;
  }
}

nsNativeThemeMac::~nsNativeThemeMac()
{
  if ( mEraseProc )
    ::DisposeThemeEraseUPP(mEraseProc);
}


void
nsNativeThemeMac::DrawCheckboxRadio ( ThemeButtonKind inKind, const Rect& inBoxRect, PRBool inChecked,
                                       PRBool inDisabled, PRInt32 inState )
{
  ThemeButtonDrawInfo info;
  if ( inDisabled )
    info.state = kThemeStateInactive;
  else
    info.state = ((inState & NS_EVENT_STATE_ACTIVE) && (inState & NS_EVENT_STATE_HOVER)) ?
                     kThemeStatePressed : kThemeStateActive;
  info.value = inChecked ? kThemeButtonOn : kThemeButtonOff;
  info.adornment = (inState & NS_EVENT_STATE_FOCUS) ? kThemeAdornmentFocus : kThemeAdornmentNone;
  
  ::DrawThemeButton ( &inBoxRect, inKind, &info, nsnull, nsnull, nsnull, 0L );
}


void
nsNativeThemeMac::DrawCheckbox ( const Rect& inBoxRect, PRBool inChecked, PRBool inDisabled, PRInt32 inState )
{
  DrawCheckboxRadio(kThemeCheckBox, inBoxRect, inChecked, inDisabled, inState);
}

void
nsNativeThemeMac::DrawSmallCheckbox ( const Rect& inBoxRect, PRBool inChecked, PRBool inDisabled, PRInt32 inState )
{
  DrawCheckboxRadio(kThemeSmallCheckBox, inBoxRect, inChecked, inDisabled, inState);
}

void
nsNativeThemeMac::DrawRadio ( const Rect& inBoxRect, PRBool inChecked, PRBool inDisabled, PRInt32 inState )
{
  DrawCheckboxRadio(kThemeRadioButton, inBoxRect, inChecked, inDisabled, inState);
}

void
nsNativeThemeMac::DrawSmallRadio ( const Rect& inBoxRect, PRBool inChecked, PRBool inDisabled, PRInt32 inState )
{
  DrawCheckboxRadio(kThemeSmallRadioButton, inBoxRect, inChecked, inDisabled, inState);
}

void
nsNativeThemeMac::DrawButton ( ThemeButtonKind inKind, const Rect& inBoxRect, PRBool inIsDefault, 
                                  PRBool inDisabled, ThemeButtonValue inValue, ThemeButtonAdornment inAdornment,
                                  PRInt32 inState )
{
  ThemeButtonDrawInfo info;

  info.value = inValue;
  info.adornment = inAdornment;

  if ( inDisabled )
    info.state = kThemeStateUnavailableInactive;
  else {
    if ((inState & NS_EVENT_STATE_ACTIVE) && (inState & NS_EVENT_STATE_HOVER))
      info.state = kThemeStatePressed;
    else
      info.state = (inKind == kThemeArrowButton) ?
                   kThemeStateInactive : kThemeStateActive;
    if ( inState & NS_EVENT_STATE_FOCUS ) {
      
      
      
      
      if (inKind != kThemePushButton || OnTigerOrLater())
        info.adornment = kThemeAdornmentFocus;
    }
    if ( inIsDefault )
      info.adornment |= kThemeAdornmentDefault;
  }
  ::DrawThemeButton ( &inBoxRect, inKind, &info, nsnull, mEraseProc, nsnull, 0L );
}

void
nsNativeThemeMac::DrawSpinButtons ( ThemeButtonKind inKind, const Rect& inBoxRect,
                                    PRBool inDisabled, ThemeDrawState inDrawState, 
                                    ThemeButtonAdornment inAdornment, PRInt32 inState )
{
  ThemeButtonDrawInfo info;

  info.state= inDrawState;
  info.value = kThemeButtonOff;
  info.adornment = inAdornment;

  if ( inDisabled )
    info.state = kThemeStateUnavailableInactive;

  ::DrawThemeButton ( &inBoxRect, inKind, &info, nsnull, mEraseProc, nsnull, 0L );
}

void
nsNativeThemeMac::DrawToolbar ( const Rect& inBoxRect )
{
#if 0
  const PRInt32 kThemeBrushToolbarBackground = 52;    
  ::SetThemeBackground(kThemeBrushToolbarBackground, 24, true);
  ::EraseRect(&inBoxRect);
  ::SetThemeBackground(kThemeBrushWhite, 24, true);
printf("told to draw at %ld %ld w %ld h %ld\n", inBoxRect.left, inBoxRect.top, inBoxRect.right-inBoxRect.left,
        inBoxRect.bottom - inBoxRect.top);
#endif
  ThemeDrawState drawState = kThemeStateActive;
  ::DrawThemeWindowHeader(&inBoxRect, drawState);
}


void
nsNativeThemeMac::DrawEditText ( const Rect& inBoxRect, PRBool inIsDisabled )
{
  Pattern whitePat;
  ::BackColor(whiteColor);
  ::BackPat(GetQDGlobalsWhite(&whitePat));
  ::EraseRect(&inBoxRect);
  
  ThemeDrawState drawState = inIsDisabled ? kThemeStateDisabled : kThemeStateActive;
  ::DrawThemeEditTextFrame(&inBoxRect, drawState);
}


void
nsNativeThemeMac::DrawListBox ( const Rect& inBoxRect, PRBool inIsDisabled )
{
  Pattern whitePat;
  ::BackColor(whiteColor);
  ::BackPat(GetQDGlobalsWhite(&whitePat));
  ::EraseRect(&inBoxRect);
  
  ThemeDrawState drawState = inIsDisabled ? kThemeStateDisabled : kThemeStateActive;
  ::DrawThemeListBoxFrame(&inBoxRect, drawState);
}


void
nsNativeThemeMac::DrawProgress ( const Rect& inBoxRect, PRBool inIsDisabled, PRBool inIsIndeterminate, 
                                  PRBool inIsHorizontal, PRInt32 inValue )
{
  ThemeTrackDrawInfo info;
  static SInt32 sPhase = 0;
  
  info.kind = inIsIndeterminate ? kThemeMediumIndeterminateBar: kThemeMediumProgressBar;
  info.bounds = inBoxRect;
  info.min = 0;
  info.max = 100;
  info.value = inValue;
  info.attributes = inIsHorizontal ? kThemeTrackHorizontal : 0L;
  info.enableState = inIsDisabled ? kThemeTrackDisabled : kThemeTrackActive;
  info.trackInfo.progress.phase = sPhase++;       
  
  ::DrawThemeTrack(&info, nsnull, nsnull, 0L);
}


void
nsNativeThemeMac::DrawTabPanel ( const Rect& inBoxRect, PRBool inIsDisabled )
{
  ThemeDrawState drawState = inIsDisabled ? kThemeStateDisabled : kThemeStateActive;
  ::DrawThemeTabPane(&inBoxRect, drawState);
}


void
nsNativeThemeMac::DrawSeparator ( const Rect& inBoxRect, PRBool inIsDisabled )
{
  ThemeDrawState drawState = inIsDisabled ? kThemeStateDisabled : kThemeStateActive;
  ::DrawThemeSeparator(&inBoxRect, drawState);
}


void
nsNativeThemeMac::DrawScale ( const Rect& inBoxRect, PRBool inIsDisabled, PRInt32 inState,
                              PRBool inIsVertical, PRInt32 inCurrentValue,
                              PRInt32 inMinValue, PRInt32 inMaxValue )
{
  ThemeTrackDrawInfo info;

  info.kind = kThemeMediumSlider;
  info.bounds = inBoxRect;
  info.min = inMinValue;
  info.max = inMaxValue;
  info.value = inCurrentValue;
  info.attributes = kThemeTrackShowThumb;
  if (!inIsVertical)
    info.attributes |= kThemeTrackHorizontal;
  if (inState & NS_EVENT_STATE_FOCUS)
    info.attributes |= kThemeTrackHasFocus;
  info.enableState = (inIsDisabled ? kThemeTrackDisabled : kThemeTrackActive);
  info.trackInfo.slider.thumbDir = kThemeThumbPlain;
  info.trackInfo.slider.pressState = 0;
  
  ::DrawThemeTrack(&info, nsnull, nsnull, 0);
}


void
nsNativeThemeMac::DrawTab ( const Rect& inBoxRect, PRBool inIsDisabled, PRBool inIsFrontmost, 
                              PRBool inIsHorizontal, PRBool inTabBottom, PRInt32 inState )
{
  ThemeTabStyle style = 0L;
  if ( inIsFrontmost ) {
    if ( inIsDisabled ) 
      style = kThemeTabFrontInactive;
    else
      style = kThemeTabFront;
  }
  else {
    if ( inIsDisabled )
      style = kThemeTabNonFrontInactive;
    else if ( (inState & NS_EVENT_STATE_ACTIVE) && (inState & NS_EVENT_STATE_HOVER) )
      style = kThemeTabNonFrontPressed;
    else
      style = kThemeTabNonFront;  
  }

  ThemeTabDirection direction = inTabBottom ? kThemeTabSouth : kThemeTabNorth; 
  ::DrawThemeTab(&inBoxRect, style, direction, nsnull, 0L);
}

void
nsNativeThemeMac::DrawMenu ( const Rect& inBoxRect, PRBool inIsDisabled )
{
  ::EraseRect(&inBoxRect);
  ThemeMenuType menuType = inIsDisabled ? kThemeMenuTypeInactive : kThemeMenuTypePopUp;
  ::DrawThemeMenuBackground(&inBoxRect, menuType);
}

void
nsNativeThemeMac::DrawMenuItem ( const Rect& inBoxRect, ThemeMenuItemType itemType, PRBool inIsDisabled,
                                   PRBool inHover)
{
  ThemeMenuState menuItemState;
  if (inIsDisabled)
    menuItemState = kThemeMenuDisabled;
  else if (inHover)
    menuItemState = kThemeMenuSelected;
  else
    menuItemState = kThemeMenuActive;

  
  ::DrawThemeMenuItem(&inBoxRect, &inBoxRect, inBoxRect.top,
                      inBoxRect.bottom, menuItemState, itemType, NULL, 0);
}

NS_IMETHODIMP
nsNativeThemeMac::DrawWidgetBackground(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                        PRUint8 aWidgetType, const nsRect& aRect, const nsRect& aClipRect)
{
  
  nsIDrawingSurface* surf;
  aContext->GetDrawingSurface(&surf);
  nsCOMPtr<nsIDrawingSurfaceMac> macSurface(do_QueryInterface(surf));
  CGrafPtr port = nsnull;
  NS_ASSERTION(macSurface,"no surface!!!\n");
  if ( macSurface )
    macSurface->GetGrafPtr(&port);
  else
    return NS_ERROR_FAILURE;      
  StPortSetter temp(port);

  
  
  
  StRegionFromPool oldClip;
  ::GetClip(oldClip);

  
  nsTransform2D* transformMatrix;
  aContext->GetCurrentTransform(transformMatrix);
  nsRect transRect(aRect), transClipRect(aClipRect);
  Rect macRect;
  transformMatrix->TransformCoord(&transRect.x, &transRect.y, &transRect.width, &transRect.height);
  ConvertGeckoToNativeRect(transRect, macRect);
#ifdef CLIP_DRAWING
  Rect clipRect;
  transformMatrix->TransformCoord(&transClipRect.x, &transClipRect.y, &transClipRect.width, &transClipRect.height);
  ConvertGeckoToNativeRect(transClipRect, clipRect);
  ::ClipRect(&clipRect);
#endif

  PRInt32 eventState = GetContentState(aFrame, aWidgetType);

  switch ( aWidgetType ) {
  
    case NS_THEME_DIALOG:
      ::SetThemeBackground(kThemeBrushDialogBackgroundActive, 24, true);
      ::EraseRect(&macRect);
      ::SetThemeBackground(kThemeBrushWhite, 24, true);
      break;

    case NS_THEME_MENUPOPUP:
      ::SetThemeBackground(kThemeBrushDialogBackgroundActive, 24, true);
      DrawMenu(macRect, IsDisabled(aFrame));
      ::SetThemeBackground(kThemeBrushWhite, 24, true);
      break;

    case NS_THEME_MENUITEM:
      ::SetThemeBackground(kThemeBrushDialogBackgroundActive, 24, true);
      DrawMenuItem(macRect, kThemeMenuItemPlain, IsDisabled(aFrame),
                   CheckBooleanAttr(aFrame, nsWidgetAtoms::mozmenuactive));
      ::SetThemeBackground(kThemeBrushWhite, 24, true);
      break;

    case NS_THEME_TOOLTIP:
    {
      RGBColor yellow = {65535,65535,45000};
      ::RGBBackColor(&yellow);
      ::EraseRect(&macRect);
      ::SetThemeBackground(kThemeBrushWhite, 24, true);
      break;
    }

    case NS_THEME_CHECKBOX:
      DrawCheckbox ( macRect, IsChecked(aFrame), IsDisabled(aFrame), eventState );
      break;    
    case NS_THEME_RADIO:
      DrawRadio ( macRect, IsSelected(aFrame), IsDisabled(aFrame), eventState );
      break;
    case NS_THEME_CHECKBOX_SMALL:
      if (transRect.height == 15) {
      	
        ++macRect.bottom;
      }
      DrawSmallCheckbox ( macRect, IsChecked(aFrame), IsDisabled(aFrame), eventState );
      break;
    case NS_THEME_RADIO_SMALL:
      if (transRect.height == 14) {
        
        ++macRect.bottom;
      }
      DrawSmallRadio ( macRect, IsSelected(aFrame), IsDisabled(aFrame), eventState );
      break;
    case NS_THEME_BUTTON:
    case NS_THEME_BUTTON_SMALL:
      DrawButton ( kThemePushButton, macRect, IsDefaultButton(aFrame), IsDisabled(aFrame), 
                    kThemeButtonOn, kThemeAdornmentNone, eventState );
      break; 
    case NS_THEME_BUTTON_BEVEL:
      DrawButton ( kThemeMediumBevelButton, macRect, IsDefaultButton(aFrame), IsDisabled(aFrame), 
                    kThemeButtonOff, kThemeAdornmentNone, eventState );
      break;
    case NS_THEME_SPINNER:
      {
        ThemeDrawState state = kThemeStateActive;
        nsIContent* content = aFrame->GetContent();
        if (content->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::state,
                                 NS_LITERAL_STRING("up"), eCaseMatters))
          state = kThemeStatePressedUp;
        else if (content->AttrValueIs(kNameSpaceID_None, nsWidgetAtoms::state,
                                      NS_LITERAL_STRING("down"), eCaseMatters))
          state = kThemeStatePressedDown;
        DrawSpinButtons ( kThemeIncDecButton, macRect, IsDisabled(aFrame),
                          state, kThemeAdornmentNone, eventState );
        break; 
      }
    case NS_THEME_TOOLBAR_BUTTON:
      DrawButton ( kThemePushButton, macRect, IsDefaultButton(aFrame), IsDisabled(aFrame),
                    kThemeButtonOn, kThemeAdornmentNone, eventState );
      break;
    case NS_THEME_TOOLBAR_SEPARATOR:
      DrawSeparator ( macRect, IsDisabled(aFrame) );
      break;
      
    case NS_THEME_TOOLBAR:
    case NS_THEME_TOOLBOX:
    case NS_THEME_STATUSBAR:
      DrawToolbar ( macRect );
      break;
      
    case NS_THEME_DROPDOWN:
      DrawButton ( kThemePopupButton, macRect, IsDefaultButton(aFrame), IsDisabled(aFrame), 
                    kThemeButtonOn, kThemeAdornmentNone, eventState );
      break;
    case NS_THEME_DROPDOWN_BUTTON:
      DrawButton ( kThemeArrowButton, macRect, PR_FALSE, IsDisabled(aFrame), 
                   kThemeButtonOn, kThemeAdornmentArrowDownArrow, eventState );
      break;

    case NS_THEME_TEXTFIELD:
      DrawEditText ( macRect, (IsDisabled(aFrame) || IsReadOnly(aFrame)) );
      break;
      
    case NS_THEME_PROGRESSBAR:
      DrawProgress ( macRect, IsDisabled(aFrame), IsIndeterminateProgress(aFrame), PR_TRUE, GetProgressValue(aFrame) );
      break;
    case NS_THEME_PROGRESSBAR_VERTICAL:
      DrawProgress ( macRect, IsDisabled(aFrame), IsIndeterminateProgress(aFrame), PR_FALSE, GetProgressValue(aFrame) );
      break;
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
      
      break;

    case NS_THEME_TREEVIEW_TWISTY:
      DrawButton ( kThemeDisclosureButton, macRect, PR_FALSE, IsDisabled(aFrame), 
                    kThemeDisclosureRight, kThemeAdornmentNone, eventState );
      break;
    case NS_THEME_TREEVIEW_TWISTY_OPEN:
      DrawButton ( kThemeDisclosureButton, macRect, PR_FALSE, IsDisabled(aFrame), 
                    kThemeDisclosureDown, kThemeAdornmentNone, eventState );
      break;
    case NS_THEME_TREEVIEW_HEADER_CELL:
    {
      TreeSortDirection sortDirection = GetTreeSortDirection(aFrame);
      DrawButton ( kThemeListHeaderButton, macRect, PR_FALSE, IsDisabled(aFrame), 
                    sortDirection == eTreeSortDirection_Natural ? kThemeButtonOff : kThemeButtonOn,
                    sortDirection == eTreeSortDirection_Descending ?
                    kThemeAdornmentHeaderButtonSortUp : kThemeAdornmentNone, eventState );      
      break;
    }
    case NS_THEME_TREEVIEW_TREEITEM:
    case NS_THEME_TREEVIEW:
      ::SetThemeBackground(kThemeBrushWhite, 24, true);
      ::EraseRect ( &macRect );
      break;
    case NS_THEME_TREEVIEW_HEADER:
      
    case NS_THEME_TREEVIEW_HEADER_SORTARROW:
      
    case NS_THEME_TREEVIEW_LINE:
      
      break;

    case NS_THEME_SCALE_HORIZONTAL:
    case NS_THEME_SCALE_VERTICAL:
    {
      PRInt32 curpos = CheckIntAttr(aFrame, nsWidgetAtoms::curpos);
      PRInt32 minpos = CheckIntAttr(aFrame, nsWidgetAtoms::minpos);
      PRInt32 maxpos = CheckIntAttr(aFrame, nsWidgetAtoms::maxpos);
      if (!maxpos)
        maxpos = 100;

      DrawScale(macRect, IsDisabled(aFrame), eventState,
                (aWidgetType == NS_THEME_SCALE_VERTICAL),
                curpos, minpos, maxpos);
      break;
    }

    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_VERTICAL:
      
      break;

    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL: 
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
      
      
      break;
    
    case NS_THEME_LISTBOX:
      DrawListBox(macRect, IsDisabled(aFrame));
      break;
    
    case NS_THEME_TAB:
      DrawTab(macRect, IsDisabled(aFrame), IsSelectedTab(aFrame), PR_TRUE, IsBottomTab(aFrame), eventState);
      break;      
    case NS_THEME_TAB_PANELS:
      DrawTabPanel(macRect, IsDisabled(aFrame));
      break;
  }

  ::SetClip(oldClip);
  
  return NS_OK;
}


NS_IMETHODIMP
nsNativeThemeMac::GetWidgetBorder(nsIDeviceContext* aContext, 
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsMargin* aResult)
{
  aResult->SizeTo(0,0,0,0);
      
  
  
  switch ( aWidgetType ) {
  
    case NS_THEME_BUTTON:
      aResult->SizeTo(kAquaPushButtonEndcaps, kAquaPushButtonTopBottom, 
                          kAquaPushButtonEndcaps, kAquaPushButtonTopBottom);
      break;

    case NS_THEME_BUTTON_SMALL:
      aResult->SizeTo(kAquaSmallPushButtonEndcaps, kAquaPushButtonTopBottom,
                      kAquaSmallPushButtonEndcaps, kAquaPushButtonTopBottom);
      break;

    case NS_THEME_TOOLBAR_BUTTON:
      
      break;

    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_BUTTON:
      aResult->SizeTo(kAquaDropdownLeftEndcap, kAquaPushButtonTopBottom, 
                        kAquaDropwdonRightEndcap, kAquaPushButtonTopBottom);
      break;
    
    case NS_THEME_TEXTFIELD:
    {
      aResult->SizeTo(2, 2, 2, 2);
      break;
    }

    case NS_THEME_LISTBOX:
    {
      SInt32 frameOutset = 0;
      ::GetThemeMetric(kThemeMetricListBoxFrameOutset, &frameOutset);
      aResult->SizeTo(frameOutset, frameOutset, frameOutset, frameOutset);
      break;
    }
      
  }
  
  return NS_OK;
}

PRBool
nsNativeThemeMac::GetWidgetPadding(nsIDeviceContext* aContext, 
                                   nsIFrame* aFrame,
                                   PRUint8 aWidgetType,
                                   nsMargin* aResult)
{
  return PR_FALSE;
}

NS_IMETHODIMP
nsNativeThemeMac::GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                       PRUint8 aWidgetType, nsSize* aResult, PRBool* aIsOverridable)
{
  
  aResult->SizeTo(0,0);
  *aIsOverridable = PR_TRUE;

  switch ( aWidgetType ) {
  
    case NS_THEME_BUTTON:
    {
      SInt32 buttonHeight = 0;
      ::GetThemeMetric(kThemeMetricPushButtonHeight, &buttonHeight);
      aResult->SizeTo(kAquaPushButtonEndcaps*2, buttonHeight);
      break;
    }
      
    case NS_THEME_BUTTON_SMALL:
    {
      SInt32 buttonHeight = 0;
      ::GetThemeMetric(kThemeMetricSmallPushButtonHeight, &buttonHeight);
      aResult->SizeTo(kAquaSmallPushButtonEndcaps*2, buttonHeight);
      break;
    }
    
    case NS_THEME_SPINNER:
    {
      SInt32 buttonHeight = 0;
      ::GetThemeMetric(kThemeMetricPushButtonHeight, &buttonHeight);
      aResult->SizeTo(kAquaPushButtonEndcaps, buttonHeight);
      break;
    }

    case NS_THEME_CHECKBOX:
    {
      SInt32 boxHeight = 0, boxWidth = 0;
      ::GetThemeMetric(kThemeMetricCheckBoxWidth, &boxWidth);
      ::GetThemeMetric(kThemeMetricCheckBoxHeight, &boxHeight);
      aResult->SizeTo(boxWidth, boxHeight);
      *aIsOverridable = PR_FALSE;
      break;
    }
    
    case NS_THEME_RADIO:
    {
      SInt32 radioHeight = 0, radioWidth = 0;
      ::GetThemeMetric(kThemeMetricRadioButtonWidth, &radioWidth);
      ::GetThemeMetric(kThemeMetricRadioButtonHeight, &radioHeight);
      aResult->SizeTo(radioWidth, radioHeight);
      *aIsOverridable = PR_FALSE;
      break;
    }

    case NS_THEME_CHECKBOX_SMALL:
    {
      
      
      
      
      
      
      

      aResult->SizeTo(14, 15);
      *aIsOverridable = PR_FALSE;
      break;
    }

    case NS_THEME_RADIO_SMALL:
    {
      
      

      aResult->SizeTo(14, 14);
      *aIsOverridable = PR_FALSE;
      break;
    }

    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_BUTTON:
    {
      SInt32 popupHeight = 0;
      ::GetThemeMetric(kThemeMetricPopupButtonHeight, &popupHeight);
      aResult->SizeTo(0, popupHeight);
      break;
    }

    case NS_THEME_TEXTFIELD:
    {
      
      
      
      aResult->SizeTo(0, (2 + 2)  + 9 + (1 + 1)  );
      break;
    }
      
    case NS_THEME_PROGRESSBAR:
    {
      SInt32 barHeight = 0;
      ::GetThemeMetric(kThemeMetricNormalProgressBarThickness, &barHeight);
      aResult->SizeTo(0, barHeight);
      break;
    }

    case NS_THEME_TREEVIEW_TWISTY:
    case NS_THEME_TREEVIEW_TWISTY_OPEN:   
    {
      SInt32 twistyHeight = 0, twistyWidth = 0;
      ::GetThemeMetric(kThemeMetricDisclosureButtonWidth, &twistyWidth);
      ::GetThemeMetric(kThemeMetricDisclosureButtonHeight, &twistyHeight);
      aResult->SizeTo(twistyWidth, twistyHeight);
      *aIsOverridable = PR_FALSE;
      break;
    }
    
    case NS_THEME_TREEVIEW_HEADER:
    case NS_THEME_TREEVIEW_HEADER_CELL:
    {
      SInt32 headerHeight = 0;
      ::GetThemeMetric(kThemeMetricListHeaderHeight, &headerHeight);
      aResult->SizeTo(0, headerHeight);
      break;
    }

    case NS_THEME_SCALE_HORIZONTAL:
    {
      SInt32 scaleHeight = 0;
      ::GetThemeMetric(kThemeMetricHSliderHeight, &scaleHeight);
      aResult->SizeTo(scaleHeight, scaleHeight);
      *aIsOverridable = PR_FALSE;
      break;
    }

    case NS_THEME_SCALE_VERTICAL:
    {
      SInt32 scaleWidth = 0;
      ::GetThemeMetric(kThemeMetricVSliderWidth, &scaleWidth);
      aResult->SizeTo(scaleWidth, scaleWidth);
      *aIsOverridable = PR_FALSE;
      break;
    }
      
    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    {
      
      
      
      
      SInt32 scrollbarWidth = 0;
      ::GetThemeMetric(kThemeMetricScrollBarWidth, &scrollbarWidth);
      aResult->SizeTo(scrollbarWidth, scrollbarWidth);
      *aIsOverridable = PR_FALSE;
      break;
    }

  }

  return NS_OK;
}


NS_IMETHODIMP
nsNativeThemeMac::WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                     nsIAtom* aAttribute, PRBool* aShouldRepaint)
{
  
  switch ( aWidgetType ) {
    case NS_THEME_TOOLBOX:
    case NS_THEME_TOOLBAR:
    case NS_THEME_TOOLBAR_BUTTON:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL: 
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_STATUSBAR:
    case NS_THEME_STATUSBAR_PANEL:
    case NS_THEME_STATUSBAR_RESIZER_PANEL:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_TOOLTIP:
    case NS_THEME_TAB_PANELS:
    case NS_THEME_TAB_PANEL:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_DIALOG:
    case NS_THEME_MENUPOPUP:
      *aShouldRepaint = PR_FALSE;
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
        aAttribute == nsWidgetAtoms::mozmenuactive ||
        aAttribute == nsWidgetAtoms::sortdirection)
      *aShouldRepaint = PR_TRUE;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsNativeThemeMac::ThemeChanged()
{
  
  return NS_OK;
}


PRBool 
nsNativeThemeMac::ThemeSupportsWidget(nsPresContext* aPresContext, nsIFrame* aFrame,
                                      PRUint8 aWidgetType)
{
#ifndef MOZ_MACBROWSER
  
  if (aFrame && aFrame->GetContent()->IsNodeOfType(nsINode::eHTML))
    return PR_FALSE;
#endif

  if (aPresContext && !aPresContext->PresShell()->IsThemeSupportEnabled())
    return PR_FALSE;

  PRBool retVal = PR_FALSE;

  switch ( aWidgetType ) {
    case NS_THEME_DIALOG:
    case NS_THEME_WINDOW:
    case NS_THEME_MENUPOPUP:
    case NS_THEME_MENUITEM:
    case NS_THEME_TOOLTIP:
    
    case NS_THEME_CHECKBOX:
    case NS_THEME_CHECKBOX_SMALL:
    case NS_THEME_CHECKBOX_CONTAINER:
    case NS_THEME_RADIO:
    case NS_THEME_RADIO_SMALL:
    case NS_THEME_RADIO_CONTAINER:
    case NS_THEME_BUTTON:
    case NS_THEME_BUTTON_SMALL:
    case NS_THEME_BUTTON_BEVEL:
    case NS_THEME_SPINNER:
    case NS_THEME_TOOLBAR:
    case NS_THEME_STATUSBAR:
    case NS_THEME_TEXTFIELD:
    
    
    case NS_THEME_PROGRESSBAR:
    case NS_THEME_PROGRESSBAR_VERTICAL:
    case NS_THEME_PROGRESSBAR_CHUNK:
    case NS_THEME_PROGRESSBAR_CHUNK_VERTICAL:
    case NS_THEME_TOOLBAR_SEPARATOR:
    
    case NS_THEME_TAB_PANELS:
    case NS_THEME_TAB:
    case NS_THEME_TAB_LEFT_EDGE:
    case NS_THEME_TAB_RIGHT_EDGE:
    
    case NS_THEME_TREEVIEW_TWISTY:
    case NS_THEME_TREEVIEW_TWISTY_OPEN:
    case NS_THEME_TREEVIEW:
    case NS_THEME_TREEVIEW_HEADER:
    case NS_THEME_TREEVIEW_HEADER_CELL:
    case NS_THEME_TREEVIEW_HEADER_SORTARROW:
    case NS_THEME_TREEVIEW_TREEITEM:
    case NS_THEME_TREEVIEW_LINE:

    case NS_THEME_SCALE_HORIZONTAL:
    case NS_THEME_SCALE_THUMB_HORIZONTAL:
    case NS_THEME_SCALE_VERTICAL:
    case NS_THEME_SCALE_THUMB_VERTICAL:

    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
      retVal = PR_TRUE;
      break;

    case NS_THEME_LISTBOX:
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_BUTTON:
    case NS_THEME_DROPDOWN_TEXT:
      
      
      return PR_TRUE;
  }

  return retVal ? !IsWidgetStyled(aPresContext, aFrame, aWidgetType) : PR_FALSE;
}


PRBool
nsNativeThemeMac::WidgetIsContainer(PRUint8 aWidgetType)
{
  
  switch ( aWidgetType ) {
   case NS_THEME_DROPDOWN_BUTTON:
   case NS_THEME_RADIO:
   case NS_THEME_CHECKBOX:
   case NS_THEME_PROGRESSBAR:
    return PR_FALSE;
    break;
  }
  return PR_TRUE;
}
