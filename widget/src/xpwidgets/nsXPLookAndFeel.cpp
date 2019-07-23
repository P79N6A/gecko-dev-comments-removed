




































#include "nscore.h"

#include "nsXPLookAndFeel.h"
#include "nsIServiceManager.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIObserver.h"
#include "nsCRT.h"
#include "nsFont.h"

#include "gfxPlatform.h"
#include "qcms.h"

#ifdef DEBUG
#include "nsSize.h"
#endif

NS_IMPL_ISUPPORTS2(nsXPLookAndFeel, nsILookAndFeel, nsIObserver)

nsLookAndFeelIntPref nsXPLookAndFeel::sIntPrefs[] =
{
  { "ui.windowTitleHeight", eMetric_WindowTitleHeight, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.windowBorderWidth", eMetric_WindowBorderWidth, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.windowBorderHeight", eMetric_WindowBorderHeight, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.widget3DBorder", eMetric_Widget3DBorder, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.textFieldBorder", eMetric_TextFieldBorder, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.textFieldHeight", eMetric_TextFieldHeight, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.buttonHorizontalInsidePaddingNavQuirks",
    eMetric_ButtonHorizontalInsidePaddingNavQuirks, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.buttonHorizontalInsidePaddingOffsetNavQuirks",
    eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.checkboxSize", eMetric_CheckboxSize, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.radioboxSize", eMetric_RadioboxSize, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.textHorizontalInsideMinimumPadding",
    eMetric_TextHorizontalInsideMinimumPadding, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.textVerticalInsidePadding", eMetric_TextVerticalInsidePadding,
    PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.textShouldUseVerticalInsidePadding",
    eMetric_TextShouldUseVerticalInsidePadding, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.textShouldUseHorizontalInsideMinimumPadding",
    eMetric_TextShouldUseHorizontalInsideMinimumPadding, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.listShouldUseHorizontalInsideMinimumPadding",
    eMetric_ListShouldUseHorizontalInsideMinimumPadding, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.listHorizontalInsideMinimumPadding",
    eMetric_ListHorizontalInsideMinimumPadding, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.listShouldUseVerticalInsidePadding",
    eMetric_ListShouldUseVerticalInsidePadding, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.listVerticalInsidePadding", eMetric_ListVerticalInsidePadding,
    PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.caretBlinkTime", eMetric_CaretBlinkTime, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.caretWidth", eMetric_CaretWidth, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.caretVisibleWithSelection", eMetric_ShowCaretDuringSelection, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.submenuDelay", eMetric_SubmenuDelay, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.dragThresholdX", eMetric_DragThresholdX, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.dragThresholdY", eMetric_DragThresholdY, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.useAccessibilityTheme", eMetric_UseAccessibilityTheme, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.isScreenReaderActive", eMetric_IsScreenReaderActive, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.menusCanOverlapOSBar", eMetric_MenusCanOverlapOSBar,
    PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.skipNavigatingDisabledMenuItem", eMetric_SkipNavigatingDisabledMenuItem, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.treeOpenDelay",
    eMetric_TreeOpenDelay, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.treeCloseDelay",
    eMetric_TreeCloseDelay, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.treeLazyScrollDelay",
    eMetric_TreeLazyScrollDelay, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.treeScrollDelay",
    eMetric_TreeScrollDelay, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.treeScrollLinesMax",
    eMetric_TreeScrollLinesMax, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "accessibility.tabfocus",
    eMetric_TabFocusModel, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.alertNotificationOrigin",
    eMetric_AlertNotificationOrigin, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.scrollToClick",
    eMetric_ScrollToClick, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.IMERawInputUnderlineStyle",
    eMetric_IMERawInputUnderlineStyle, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.IMESelectedRawTextUnderlineStyle",
    eMetric_IMESelectedRawTextUnderlineStyle, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.IMEConvertedTextUnderlineStyle",
    eMetric_IMEConvertedTextUnderlineStyle, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.IMESelectedConvertedTextUnderlineStyle",
    eMetric_IMESelectedConvertedTextUnderline, PR_FALSE, nsLookAndFeelTypeInt, 0 },
  { "ui.SpellCheckerUnderlineStyle",
    eMetric_SpellCheckerUnderlineStyle, PR_FALSE, nsLookAndFeelTypeInt, 0 },
};

nsLookAndFeelFloatPref nsXPLookAndFeel::sFloatPrefs[] =
{
  { "ui.textFieldVerticalInsidePadding",
    eMetricFloat_TextFieldVerticalInsidePadding, PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.textFieldHorizontalInsidePadding",
    eMetricFloat_TextFieldHorizontalInsidePadding, PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.textAreaVerticalInsidePadding", eMetricFloat_TextAreaVerticalInsidePadding,
    PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.textAreaHorizontalInsidePadding",
    eMetricFloat_TextAreaHorizontalInsidePadding, PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.listVerticalInsidePadding",
    eMetricFloat_ListVerticalInsidePadding, PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.listHorizontalInsidePadding",
    eMetricFloat_ListHorizontalInsidePadding, PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.buttonVerticalInsidePadding", eMetricFloat_ButtonVerticalInsidePadding,
    PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.buttonHorizontalInsidePadding", eMetricFloat_ButtonHorizontalInsidePadding,
    PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.IMEUnderlineRelativeSize", eMetricFloat_IMEUnderlineRelativeSize,
    PR_FALSE, nsLookAndFeelTypeFloat, 0 },
  { "ui.SpellCheckerUnderlineRelativeSize",
    eMetricFloat_SpellCheckerUnderlineRelativeSize, PR_FALSE,
    nsLookAndFeelTypeFloat, 0 },
  { "ui.caretAspectRatio", eMetricFloat_CaretAspectRatio, PR_FALSE,
    nsLookAndFeelTypeFloat, 0 },
};








const char nsXPLookAndFeel::sColorPrefs[][38] =
{
  "ui.windowBackground",
  "ui.windowForeground",
  "ui.widgetBackground",
  "ui.widgetForeground",
  "ui.widgetSelectBackground",
  "ui.widgetSelectForeground",
  "ui.widget3DHighlight",
  "ui.widget3DShadow",
  "ui.textBackground",
  "ui.textForeground",
  "ui.textSelectBackground",
  "ui.textSelectForeground",
  "ui.textSelectBackgroundDisabled",
  "ui.textSelectBackgroundAttention",
  "ui.textHighlightBackground",
  "ui.textHighlightForeground",
  "ui.IMERawInputBackground",
  "ui.IMERawInputForeground",
  "ui.IMERawInputUnderline",
  "ui.IMESelectedRawTextBackground",
  "ui.IMESelectedRawTextForeground",
  "ui.IMESelectedRawTextUnderline",
  "ui.IMEConvertedTextBackground",
  "ui.IMEConvertedTextForeground",
  "ui.IMEConvertedTextUnderline",
  "ui.IMESelectedConvertedTextBackground",
  "ui.IMESelectedConvertedTextForeground",
  "ui.IMESelectedConvertedTextUnderline",
  "ui.SpellCheckerUnderline",
  "ui.activeborder",
  "ui.activecaption",
  "ui.appworkspace",
  "ui.background",
  "ui.buttonface",
  "ui.buttonhighlight",
  "ui.buttonshadow",
  "ui.buttontext",
  "ui.captiontext",
  "ui.graytext",
  "ui.highlight",
  "ui.highlighttext",
  "ui.inactiveborder",
  "ui.inactivecaption",
  "ui.inactivecaptiontext",
  "ui.infobackground",
  "ui.infotext",
  "ui.menu",
  "ui.menutext",
  "ui.scrollbar",
  "ui.threeddarkshadow",
  "ui.threedface",
  "ui.threedhighlight",
  "ui.threedlightshadow",
  "ui.threedshadow",
  "ui.window",
  "ui.windowframe",
  "ui.windowtext",
  "ui.-moz-buttondefault",
  "ui.-moz-field",
  "ui.-moz-fieldtext",
  "ui.-moz-dialog",
  "ui.-moz-dialogtext",
  "ui.-moz-dragtargetzone",
  "ui.-moz-cellhighlight",
  "ui.-moz_cellhighlighttext",
  "ui.-moz-html-cellhighlight",
  "ui.-moz-html-cellhighlighttext",
  "ui.-moz-buttonhoverface",
  "ui.-moz_buttonhovertext",
  "ui.-moz_menuhover",
  "ui.-moz_menuhovertext",
  "ui.-moz_menubartext",
  "ui.-moz_menubarhovertext",
  "ui.-moz_eventreerow",
  "ui.-moz_oddtreerow",
  "ui.-moz_mac_chrome_active",
  "ui.-moz_mac_chrome_inactive",
  "ui.-moz-mac-focusring",
  "ui.-moz-mac-menuselect",
  "ui.-moz-mac-menushadow",
  "ui.-moz-mac-menutextdisable",
  "ui.-moz-mac-menutextselect",
  "ui.-moz_mac_disabledtoolbartext",
  "ui.-moz-mac-alternateprimaryhighlight",
  "ui.-moz-mac-secondaryhighlight",
  "ui.-moz-win-mediatext",
  "ui.-moz-win-communicationstext",
  "ui.-moz-nativehyperlinktext",
  "ui.-moz-comboboxtext",
  "ui.-moz-combobox"
};

PRInt32 nsXPLookAndFeel::sCachedColors[nsILookAndFeel::eColor_LAST_COLOR] = {0};
PRInt32 nsXPLookAndFeel::sCachedColorBits[COLOR_CACHE_SIZE] = {0};

PRBool nsXPLookAndFeel::sInitialized = PR_FALSE;
PRBool nsXPLookAndFeel::sUseNativeColors = PR_TRUE;

nsXPLookAndFeel::nsXPLookAndFeel() : nsILookAndFeel()
{
}

void
nsXPLookAndFeel::IntPrefChanged (nsLookAndFeelIntPref *data)
{
  if (data)
  {
    nsCOMPtr<nsIPrefBranch> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (prefService)
    {
      PRInt32 intpref;
      nsresult rv = prefService->GetIntPref(data->name, &intpref);
      if (NS_SUCCEEDED(rv))
      {
        data->intVar = intpref;
        data->isSet = PR_TRUE;
#ifdef DEBUG_akkana
        printf("====== Changed int pref %s to %d\n", data->name, data->intVar);
#endif
      }
    }
  }
}

void
nsXPLookAndFeel::FloatPrefChanged (nsLookAndFeelFloatPref *data)
{
  if (data)
  {
    nsCOMPtr<nsIPrefBranch> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (prefService)
    {
      PRInt32 intpref;
      nsresult rv = prefService->GetIntPref(data->name, &intpref);
      if (NS_SUCCEEDED(rv))
      {
        data->floatVar = (float)intpref / 100.;
        data->isSet = PR_TRUE;
#ifdef DEBUG_akkana
        printf("====== Changed float pref %s to %f\n", data->name, data->floatVar);
#endif
      }
    }
  }
}

void
nsXPLookAndFeel::ColorPrefChanged (unsigned int index, const char *prefName)
{
  nsCOMPtr<nsIPrefBranch> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefService) {
    nsXPIDLCString colorStr;
    nsresult rv = prefService->GetCharPref(prefName, getter_Copies(colorStr));
    if (NS_SUCCEEDED(rv) && !colorStr.IsEmpty()) {
      nscolor thecolor;
      if (colorStr[0] == '#') {
        if (NS_SUCCEEDED(NS_HexToRGB(NS_ConvertASCIItoUTF16(Substring(colorStr, 1, colorStr.Length() - 1)),
                                     &thecolor))) {
          PRInt32 id = NS_PTR_TO_INT32(index);
          CACHE_COLOR(id, thecolor);
        }
      }
      else if (NS_SUCCEEDED(NS_ColorNameToRGB(NS_ConvertASCIItoUTF16(colorStr),
                                         &thecolor))) {
        PRInt32 id = NS_PTR_TO_INT32(index);
        CACHE_COLOR(id, thecolor);
#ifdef DEBUG_akkana
        printf("====== Changed color pref %s to 0x%lx\n",
               prefName, thecolor);
#endif
      }
    } else if (colorStr.IsEmpty()) {
      
      
      PRInt32 id = NS_PTR_TO_INT32(index);
      CLEAR_COLOR_CACHE(id);
    }
  }
}

void
nsXPLookAndFeel::InitFromPref(nsLookAndFeelIntPref* aPref, nsIPrefBranch* aPrefBranch)
{
  PRInt32 intpref;
  nsresult rv = aPrefBranch->GetIntPref(aPref->name, &intpref);
  if (NS_SUCCEEDED(rv))
  {
    aPref->isSet = PR_TRUE;
    aPref->intVar = intpref;
  }
}

void
nsXPLookAndFeel::InitFromPref(nsLookAndFeelFloatPref* aPref, nsIPrefBranch* aPrefBranch)
{
  PRInt32 intpref;
  nsresult rv = aPrefBranch->GetIntPref(aPref->name, &intpref);
  if (NS_SUCCEEDED(rv))
  {
    aPref->isSet = PR_TRUE;
    aPref->floatVar = (float)intpref / 100.;
  }
}

void
nsXPLookAndFeel::InitColorFromPref(PRInt32 i, nsIPrefBranch* aPrefBranch)
{
  nsXPIDLCString colorStr;
  nsresult rv = aPrefBranch->GetCharPref(sColorPrefs[i], getter_Copies(colorStr));
  if (NS_SUCCEEDED(rv) && !colorStr.IsEmpty())
  {
    nsAutoString colorNSStr; colorNSStr.AssignWithConversion(colorStr);
    nscolor thecolor;
    if (colorNSStr[0] == '#') {
      nsAutoString hexString;
      colorNSStr.Right(hexString, colorNSStr.Length() - 1);
      if (NS_SUCCEEDED(NS_HexToRGB(hexString, &thecolor))) {
        CACHE_COLOR(i, thecolor);
      }
    }
    else if (NS_SUCCEEDED(NS_ColorNameToRGB(colorNSStr, &thecolor)))
    {
      CACHE_COLOR(i, thecolor);
    }
  }
}

NS_IMETHODIMP
nsXPLookAndFeel::Observe(nsISupports*     aSubject,
                         const char*      aTopic,
                         const PRUnichar* aData)
{

  

  unsigned int i;
  for (i = 0; i < NS_ARRAY_LENGTH(sIntPrefs); ++i) {
    if (nsDependentString(aData).EqualsASCII(sIntPrefs[i].name)) {
      IntPrefChanged(&sIntPrefs[i]);
      return NS_OK;
    }
  }

  for (i = 0; i < NS_ARRAY_LENGTH(sFloatPrefs); ++i) {
    if (nsDependentString(aData).EqualsASCII(sFloatPrefs[i].name)) {
      FloatPrefChanged(&sFloatPrefs[i]);
      return NS_OK;
    }
  }

  for (i = 0; i < NS_ARRAY_LENGTH(sColorPrefs); ++i) {
    if (nsDependentString(aData).EqualsASCII(sColorPrefs[i])) {
      ColorPrefChanged(i, sColorPrefs[i]);
      return NS_OK;
    }
  }

  return NS_OK;
}







void
nsXPLookAndFeel::Init()
{
  
  
  sInitialized = PR_TRUE;

  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefs)
    return;
  nsCOMPtr<nsIPrefBranch2> prefBranchInternal(do_QueryInterface(prefs));
  if (!prefBranchInternal)
    return;

  unsigned int i;
  for (i = 0; i < NS_ARRAY_LENGTH(sIntPrefs); ++i) {
    InitFromPref(&sIntPrefs[i], prefs);
    prefBranchInternal->AddObserver(sIntPrefs[i].name, this, PR_FALSE);
  }

  for (i = 0; i < NS_ARRAY_LENGTH(sFloatPrefs); ++i) {
    InitFromPref(&sFloatPrefs[i], prefs);
    prefBranchInternal->AddObserver(sFloatPrefs[i].name, this, PR_FALSE);
  }

  for (i = 0; i < NS_ARRAY_LENGTH(sColorPrefs); ++i) {
    InitColorFromPref(i, prefs);
    prefBranchInternal->AddObserver(sColorPrefs[i], this, PR_FALSE);
  }

  PRBool val;
  if (NS_SUCCEEDED(prefs->GetBoolPref("ui.use_native_colors", &val))) {
    sUseNativeColors = val;
  }
}

nsXPLookAndFeel::~nsXPLookAndFeel()
{
}

PRBool
nsXPLookAndFeel::IsSpecialColor(const nsColorID aID, nscolor &aColor)
{
  switch (aID) {
    case eColor_TextSelectForeground:
      return (aColor == NS_DONT_CHANGE_COLOR);
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
    case eColor_IMERawInputBackground:
    case eColor_IMEConvertedTextBackground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
    case eColor_IMERawInputForeground:
    case eColor_IMEConvertedTextForeground:
    case eColor_IMERawInputUnderline:
    case eColor_IMEConvertedTextUnderline:
    case eColor_IMESelectedRawTextUnderline:
    case eColor_IMESelectedConvertedTextUnderline:
    case eColor_SpellCheckerUnderline:
      return NS_IS_SELECTION_SPECIAL_COLOR(aColor);
    default:
      



      return PR_FALSE;
  }
  return PR_FALSE;
}







NS_IMETHODIMP
nsXPLookAndFeel::GetColor(const nsColorID aID, nscolor &aColor)
{
  if (!sInitialized)
    Init();

  
  
  
  
  
#undef DEBUG_SYSTEM_COLOR_USE

#ifdef DEBUG_SYSTEM_COLOR_USE
  {
    nsresult rv = NS_OK;
    switch (aID) {
        
      case eColor_activecaption:
          
      case eColor_captiontext:
          
        aColor = NS_RGB(0xff, 0x00, 0x00);
        break;

      case eColor_highlight:
          
      case eColor_highlighttext:
          
        aColor = NS_RGB(0xff, 0xff, 0x00);
        break;

      case eColor_inactivecaption:
          
      case eColor_inactivecaptiontext:
          
        aColor = NS_RGB(0x66, 0x66, 0x00);
        break;

      case eColor_infobackground:
          
      case eColor_infotext:
          
        aColor = NS_RGB(0x00, 0xff, 0x00);
        break;

      case eColor_menu:
          
      case eColor_menutext:
          
        aColor = NS_RGB(0x00, 0xff, 0xff);
        break;

      case eColor_threedface:
      case eColor_buttonface:
          
      case eColor_buttontext:
          
        aColor = NS_RGB(0x00, 0x66, 0x66);
        break;

      case eColor_window:
      case eColor_windowtext:
        aColor = NS_RGB(0x00, 0x00, 0xff);
        break;

      
      

      case eColor__moz_field:
      case eColor__moz_fieldtext:
        aColor = NS_RGB(0xff, 0x00, 0xff);
        break;

      case eColor__moz_dialog:
      case eColor__moz_dialogtext:
        aColor = NS_RGB(0x66, 0x00, 0x66);
        break;

      default:
        rv = NS_ERROR_NOT_AVAILABLE;
    }
    if (NS_SUCCEEDED(rv))
      return rv;
  }
#endif 

  if (IS_COLOR_CACHED(aID)) {
    aColor = sCachedColors[aID];
    return NS_OK;
  }

  
  if (aID == eColor_TextSelectBackgroundDisabled) {
    
    
    aColor = NS_RGB(0xb0, 0xb0, 0xb0);
    return NS_OK;
  }

  if (aID == eColor_TextSelectBackgroundAttention) {
    
    
    aColor = NS_RGB(0x38, 0xd8, 0x78);
    return NS_OK;
  }

  if (aID == eColor_TextHighlightBackground) {
    
    
    aColor = NS_RGB(0xef, 0x0f, 0xff);
    return NS_OK;
  }

  if (aID == eColor_TextHighlightForeground) {
    
    
    aColor = NS_RGB(0xff, 0xff, 0xff);
    return NS_OK;
  }

  if (sUseNativeColors && NS_SUCCEEDED(NativeGetColor(aID, aColor))) {
    if ((gfxPlatform::GetCMSMode() == eCMSMode_All) && !IsSpecialColor(aID, aColor)) {
      qcms_transform *transform = gfxPlatform::GetCMSInverseRGBTransform();
      if (transform) {
        PRUint8 color[3];
        color[0] = NS_GET_R(aColor);
        color[1] = NS_GET_G(aColor);
        color[2] = NS_GET_B(aColor);
        qcms_transform_data(transform, color, color, 1);
        aColor = NS_RGB(color[0], color[1], color[2]);
      }
    }

    CACHE_COLOR(aID, aColor);
    return NS_OK;
  }

  return NS_ERROR_NOT_AVAILABLE;
}
  
NS_IMETHODIMP
nsXPLookAndFeel::GetMetric(const nsMetricID aID, PRInt32& aMetric)
{
  if (!sInitialized)
    Init();

  
  
  switch (aID) {
    case eMetric_ScrollButtonLeftMouseButtonAction:
      aMetric = 0;
      return NS_OK;
    case eMetric_ScrollButtonMiddleMouseButtonAction:
      aMetric = 3;
      return NS_OK;
    case eMetric_ScrollButtonRightMouseButtonAction:
      aMetric = 3;
      return NS_OK;
    default:
      



    break;
  }

  for (unsigned int i = 0; i < ((sizeof (sIntPrefs) / sizeof (*sIntPrefs))); ++i)
    if (sIntPrefs[i].isSet && (sIntPrefs[i].id == aID))
    {
      aMetric = sIntPrefs[i].intVar;
      return NS_OK;
    }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsXPLookAndFeel::GetMetric(const nsMetricFloatID aID, float& aMetric)
{
  if (!sInitialized)
    Init();

  for (unsigned int i = 0; i < ((sizeof (sFloatPrefs) / sizeof (*sFloatPrefs))); ++i)
    if (sFloatPrefs[i].isSet && sFloatPrefs[i].id == aID)
    {
      aMetric = sFloatPrefs[i].floatVar;
      return NS_OK;
    }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsXPLookAndFeel::LookAndFeelChanged()
{
  
  PRUint32 i;
  for (i = 0; i < nsILookAndFeel::eColor_LAST_COLOR; i++)
    sCachedColors[i] = 0;
  for (i = 0; i < COLOR_CACHE_SIZE; i++)
    sCachedColorBits[i] = 0;
  return NS_OK;
}


#ifdef DEBUG
  
  
  
  
NS_IMETHODIMP
nsXPLookAndFeel::GetNavSize(const nsMetricNavWidgetID aWidgetID,
                            const nsMetricNavFontID   aFontID, 
                            const PRInt32             aFontSize, 
                            nsSize &aSize)
{
  aSize.width  = 0;
  aSize.height = 0;
  return NS_ERROR_NOT_IMPLEMENTED;
}
#endif
