




































#include "nscore.h"

#include "nsXPLookAndFeel.h"
#include "nsLookAndFeel.h"
#include "nsCRT.h"
#include "nsFont.h"
#include "mozilla/Preferences.h"

#include "gfxPlatform.h"
#include "qcms.h"

#ifdef DEBUG
#include "nsSize.h"
#endif

using namespace mozilla;

nsLookAndFeelIntPref nsXPLookAndFeel::sIntPrefs[] =
{
  { "ui.caretBlinkTime",
    eIntID_CaretBlinkTime,
    PR_FALSE, 0 },
  { "ui.caretWidth",
    eIntID_CaretWidth,
    PR_FALSE, 0 },
  { "ui.caretVisibleWithSelection",
    eIntID_ShowCaretDuringSelection,
    PR_FALSE, 0 },
  { "ui.submenuDelay",
    eIntID_SubmenuDelay,
    PR_FALSE, 0 },
  { "ui.dragThresholdX",
    eIntID_DragThresholdX,
    PR_FALSE, 0 },
  { "ui.dragThresholdY",
    eIntID_DragThresholdY,
    PR_FALSE, 0 },
  { "ui.useAccessibilityTheme",
    eIntID_UseAccessibilityTheme,
    PR_FALSE, 0 },
  { "ui.scrollbarsCanOverlapContent",
    eIntID_ScrollbarsCanOverlapContent,
    PR_FALSE, 0 },
  { "ui.menusCanOverlapOSBar",
    eIntID_MenusCanOverlapOSBar,
    PR_FALSE, 0 },
  { "ui.skipNavigatingDisabledMenuItem",
    eIntID_SkipNavigatingDisabledMenuItem,
    PR_FALSE, 0 },
  { "ui.treeOpenDelay",
    eIntID_TreeOpenDelay,
    PR_FALSE, 0 },
  { "ui.treeCloseDelay",
    eIntID_TreeCloseDelay,
    PR_FALSE, 0 },
  { "ui.treeLazyScrollDelay",
    eIntID_TreeLazyScrollDelay,
    PR_FALSE, 0 },
  { "ui.treeScrollDelay",
    eIntID_TreeScrollDelay,
    PR_FALSE, 0 },
  { "ui.treeScrollLinesMax",
    eIntID_TreeScrollLinesMax,
    PR_FALSE, 0 },
  { "accessibility.tabfocus",
    eIntID_TabFocusModel,
    PR_FALSE, 0 },
  { "ui.alertNotificationOrigin",
    eIntID_AlertNotificationOrigin,
    PR_FALSE, 0 },
  { "ui.scrollToClick",
    eIntID_ScrollToClick,
    PR_FALSE, 0 },
  { "ui.IMERawInputUnderlineStyle",
    eIntID_IMERawInputUnderlineStyle,
    PR_FALSE, 0 },
  { "ui.IMESelectedRawTextUnderlineStyle",
    eIntID_IMESelectedRawTextUnderlineStyle,
    PR_FALSE, 0 },
  { "ui.IMEConvertedTextUnderlineStyle",
    eIntID_IMEConvertedTextUnderlineStyle,
    PR_FALSE, 0 },
  { "ui.IMESelectedConvertedTextUnderlineStyle",
    eIntID_IMESelectedConvertedTextUnderline,
    PR_FALSE, 0 },
  { "ui.SpellCheckerUnderlineStyle",
    eIntID_SpellCheckerUnderlineStyle,
    PR_FALSE, 0 },
};

nsLookAndFeelFloatPref nsXPLookAndFeel::sFloatPrefs[] =
{
  { "ui.IMEUnderlineRelativeSize",
    eFloatID_IMEUnderlineRelativeSize,
    PR_FALSE, 0 },
  { "ui.SpellCheckerUnderlineRelativeSize",
    eFloatID_SpellCheckerUnderlineRelativeSize,
    PR_FALSE, 0 },
  { "ui.caretAspectRatio",
    eFloatID_CaretAspectRatio,
    PR_FALSE, 0 },
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

PRInt32 nsXPLookAndFeel::sCachedColors[LookAndFeel::eColorID_LAST_COLOR] = {0};
PRInt32 nsXPLookAndFeel::sCachedColorBits[COLOR_CACHE_SIZE] = {0};

PRBool nsXPLookAndFeel::sInitialized = PR_FALSE;
PRBool nsXPLookAndFeel::sUseNativeColors = PR_TRUE;

nsLookAndFeel* nsXPLookAndFeel::sInstance = nsnull;
PRBool nsXPLookAndFeel::sShutdown = PR_FALSE;


nsLookAndFeel*
nsXPLookAndFeel::GetInstance()
{
  if (sInstance) {
    return sInstance;
  }

  NS_ENSURE_TRUE(!sShutdown, nsnull);

  sInstance = new nsLookAndFeel();
  return sInstance;
}


void
nsXPLookAndFeel::Shutdown()
{
  if (sShutdown) {
    return;
  }
  sShutdown = PR_TRUE;
  delete sInstance;
  sInstance = nsnull;
}

nsXPLookAndFeel::nsXPLookAndFeel() : LookAndFeel()
{
}


void
nsXPLookAndFeel::IntPrefChanged(nsLookAndFeelIntPref *data)
{
  if (!data) {
    return;
  }

  PRInt32 intpref;
  nsresult rv = Preferences::GetInt(data->name, &intpref);
  if (NS_FAILED(rv)) {
    return;
  }
  data->intVar = intpref;
  data->isSet = PR_TRUE;
#ifdef DEBUG_akkana
  printf("====== Changed int pref %s to %d\n", data->name, data->intVar);
#endif
}


void
nsXPLookAndFeel::FloatPrefChanged(nsLookAndFeelFloatPref *data)
{
  if (!data) {
    return;
  }

  PRInt32 intpref;
  nsresult rv = Preferences::GetInt(data->name, &intpref);
  if (NS_FAILED(rv)) {
    return;
  }
  data->floatVar = (float)intpref / 100.0f;
  data->isSet = PR_TRUE;
#ifdef DEBUG_akkana
  printf("====== Changed float pref %s to %f\n", data->name, data->floatVar);
#endif
}


void
nsXPLookAndFeel::ColorPrefChanged (unsigned int index, const char *prefName)
{
  nsAutoString colorStr;
  nsresult rv = Preferences::GetString(prefName, &colorStr);
  if (NS_FAILED(rv)) {
    return;
  }
  if (!colorStr.IsEmpty()) {
    nscolor thecolor;
    if (colorStr[0] == PRUnichar('#')) {
      if (NS_HexToRGB(nsDependentString(colorStr, 1), &thecolor)) {
        PRInt32 id = NS_PTR_TO_INT32(index);
        CACHE_COLOR(id, thecolor);
      }
    } else if (NS_ColorNameToRGB(colorStr, &thecolor)) {
      PRInt32 id = NS_PTR_TO_INT32(index);
      CACHE_COLOR(id, thecolor);
#ifdef DEBUG_akkana
      printf("====== Changed color pref %s to 0x%lx\n",
             prefName, thecolor);
#endif
    }
  } else {
    
    
    PRInt32 id = NS_PTR_TO_INT32(index);
    CLEAR_COLOR_CACHE(id);
  }
}

void
nsXPLookAndFeel::InitFromPref(nsLookAndFeelIntPref* aPref)
{
  PRInt32 intpref;
  nsresult rv = Preferences::GetInt(aPref->name, &intpref);
  if (NS_SUCCEEDED(rv)) {
    aPref->isSet = PR_TRUE;
    aPref->intVar = intpref;
  }
}

void
nsXPLookAndFeel::InitFromPref(nsLookAndFeelFloatPref* aPref)
{
  PRInt32 intpref;
  nsresult rv = Preferences::GetInt(aPref->name, &intpref);
  if (NS_SUCCEEDED(rv)) {
    aPref->isSet = PR_TRUE;
    aPref->floatVar = (float)intpref / 100.0f;
  }
}

void
nsXPLookAndFeel::InitColorFromPref(PRInt32 i)
{
  nsAutoString colorStr;
  nsresult rv = Preferences::GetString(sColorPrefs[i], &colorStr);
  if (NS_FAILED(rv) || colorStr.IsEmpty()) {
    return;
  }
  nscolor thecolor;
  if (colorStr[0] == PRUnichar('#')) {
    nsAutoString hexString;
    colorStr.Right(hexString, colorStr.Length() - 1);
    if (NS_HexToRGB(hexString, &thecolor)) {
      CACHE_COLOR(i, thecolor);
    }
  } else if (NS_ColorNameToRGB(colorStr, &thecolor)) {
    CACHE_COLOR(i, thecolor);
  }
}


int
nsXPLookAndFeel::OnPrefChanged(const char* aPref, void* aClosure)
{

  

  nsDependentCString prefName(aPref);
  unsigned int i;
  for (i = 0; i < NS_ARRAY_LENGTH(sIntPrefs); ++i) {
    if (prefName.Equals(sIntPrefs[i].name)) {
      IntPrefChanged(&sIntPrefs[i]);
      return 0;
    }
  }

  for (i = 0; i < NS_ARRAY_LENGTH(sFloatPrefs); ++i) {
    if (prefName.Equals(sFloatPrefs[i].name)) {
      FloatPrefChanged(&sFloatPrefs[i]);
      return 0;
    }
  }

  for (i = 0; i < NS_ARRAY_LENGTH(sColorPrefs); ++i) {
    if (prefName.Equals(sColorPrefs[i])) {
      ColorPrefChanged(i, sColorPrefs[i]);
      return 0;
    }
  }

  return 0;
}







void
nsXPLookAndFeel::Init()
{
  
  
  sInitialized = PR_TRUE;

  
  
  
  Preferences::RegisterCallback(OnPrefChanged, "ui.");
  Preferences::RegisterCallback(OnPrefChanged, "accessibility.tabfocus");

  unsigned int i;
  for (i = 0; i < NS_ARRAY_LENGTH(sIntPrefs); ++i) {
    InitFromPref(&sIntPrefs[i]);
  }

  for (i = 0; i < NS_ARRAY_LENGTH(sFloatPrefs); ++i) {
    InitFromPref(&sFloatPrefs[i]);
  }

  for (i = 0; i < NS_ARRAY_LENGTH(sColorPrefs); ++i) {
    InitColorFromPref(i);
  }

  PRBool val;
  if (NS_SUCCEEDED(Preferences::GetBool("ui.use_native_colors", &val))) {
    sUseNativeColors = val;
  }
}

nsXPLookAndFeel::~nsXPLookAndFeel()
{
  NS_ASSERTION(sInstance == this,
               "This destroying instance isn't the singleton instance");
  sInstance = nsnull;
}

PRBool
nsXPLookAndFeel::IsSpecialColor(ColorID aID, nscolor &aColor)
{
  switch (aID) {
    case eColorID_TextSelectForeground:
      return (aColor == NS_DONT_CHANGE_COLOR);
    case eColorID_IMESelectedRawTextBackground:
    case eColorID_IMESelectedConvertedTextBackground:
    case eColorID_IMERawInputBackground:
    case eColorID_IMEConvertedTextBackground:
    case eColorID_IMESelectedRawTextForeground:
    case eColorID_IMESelectedConvertedTextForeground:
    case eColorID_IMERawInputForeground:
    case eColorID_IMEConvertedTextForeground:
    case eColorID_IMERawInputUnderline:
    case eColorID_IMEConvertedTextUnderline:
    case eColorID_IMESelectedRawTextUnderline:
    case eColorID_IMESelectedConvertedTextUnderline:
    case eColorID_SpellCheckerUnderline:
      return NS_IS_SELECTION_SPECIAL_COLOR(aColor);
    default:
      



      return PR_FALSE;
  }
  return PR_FALSE;
}







nsresult
nsXPLookAndFeel::GetColorImpl(ColorID aID, nscolor &aResult)
{
  if (!sInitialized)
    Init();

  
  
  
  
  
#undef DEBUG_SYSTEM_COLOR_USE

#ifdef DEBUG_SYSTEM_COLOR_USE
  {
    nsresult rv = NS_OK;
    switch (aID) {
        
      case eColorID_activecaption:
          
      case eColorID_captiontext:
          
        aResult = NS_RGB(0xff, 0x00, 0x00);
        break;

      case eColorID_highlight:
          
      case eColorID_highlighttext:
          
        aResult = NS_RGB(0xff, 0xff, 0x00);
        break;

      case eColorID_inactivecaption:
          
      case eColorID_inactivecaptiontext:
          
        aResult = NS_RGB(0x66, 0x66, 0x00);
        break;

      case eColorID_infobackground:
          
      case eColorID_infotext:
          
        aResult = NS_RGB(0x00, 0xff, 0x00);
        break;

      case eColorID_menu:
          
      case eColorID_menutext:
          
        aResult = NS_RGB(0x00, 0xff, 0xff);
        break;

      case eColorID_threedface:
      case eColorID_buttonface:
          
      case eColorID_buttontext:
          
        aResult = NS_RGB(0x00, 0x66, 0x66);
        break;

      case eColorID_window:
      case eColorID_windowtext:
        aResult = NS_RGB(0x00, 0x00, 0xff);
        break;

      
      

      case eColorID__moz_field:
      case eColorID__moz_fieldtext:
        aResult = NS_RGB(0xff, 0x00, 0xff);
        break;

      case eColorID__moz_dialog:
      case eColorID__moz_dialogtext:
        aResult = NS_RGB(0x66, 0x00, 0x66);
        break;

      default:
        rv = NS_ERROR_NOT_AVAILABLE;
    }
    if (NS_SUCCEEDED(rv))
      return rv;
  }
#endif 

  if (IS_COLOR_CACHED(aID)) {
    aResult = sCachedColors[aID];
    return NS_OK;
  }

  
  if (aID == eColorID_TextSelectBackgroundDisabled) {
    
    
    aResult = NS_RGB(0xb0, 0xb0, 0xb0);
    return NS_OK;
  }

  if (aID == eColorID_TextSelectBackgroundAttention) {
    
    
    aResult = NS_RGB(0x38, 0xd8, 0x78);
    return NS_OK;
  }

  if (aID == eColorID_TextHighlightBackground) {
    
    
    aResult = NS_RGB(0xef, 0x0f, 0xff);
    return NS_OK;
  }

  if (aID == eColorID_TextHighlightForeground) {
    
    
    aResult = NS_RGB(0xff, 0xff, 0xff);
    return NS_OK;
  }

  if (sUseNativeColors && NS_SUCCEEDED(NativeGetColor(aID, aResult))) {
    if ((gfxPlatform::GetCMSMode() == eCMSMode_All) &&
         !IsSpecialColor(aID, aResult)) {
      qcms_transform *transform = gfxPlatform::GetCMSInverseRGBTransform();
      if (transform) {
        PRUint8 color[3];
        color[0] = NS_GET_R(aResult);
        color[1] = NS_GET_G(aResult);
        color[2] = NS_GET_B(aResult);
        qcms_transform_data(transform, color, color, 1);
        aResult = NS_RGB(color[0], color[1], color[2]);
      }
    }

    CACHE_COLOR(aID, aResult);
    return NS_OK;
  }

  return NS_ERROR_NOT_AVAILABLE;
}
  
nsresult
nsXPLookAndFeel::GetIntImpl(IntID aID, PRInt32 &aResult)
{
  if (!sInitialized)
    Init();

  
  
  switch (aID) {
    case eIntID_ScrollButtonLeftMouseButtonAction:
      aResult = 0;
      return NS_OK;
    case eIntID_ScrollButtonMiddleMouseButtonAction:
      aResult = 3;
      return NS_OK;
    case eIntID_ScrollButtonRightMouseButtonAction:
      aResult = 3;
      return NS_OK;
    default:
      



    break;
  }

  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(sIntPrefs); ++i) {
    if (sIntPrefs[i].isSet && (sIntPrefs[i].id == aID)) {
      aResult = sIntPrefs[i].intVar;
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

nsresult
nsXPLookAndFeel::GetFloatImpl(FloatID aID, float &aResult)
{
  if (!sInitialized)
    Init();

  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(sFloatPrefs); ++i) {
    if (sFloatPrefs[i].isSet && sFloatPrefs[i].id == aID) {
      aResult = sFloatPrefs[i].floatVar;
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

void
nsXPLookAndFeel::RefreshImpl()
{
  
  PRUint32 i;
  for (i = 0; i < eColorID_LAST_COLOR; i++)
    sCachedColors[i] = 0;
  for (i = 0; i < COLOR_CACHE_SIZE; i++)
    sCachedColorBits[i] = 0;
}

namespace mozilla {


nsresult
LookAndFeel::GetColor(ColorID aID, nscolor* aResult)
{
  return nsLookAndFeel::GetInstance()->GetColorImpl(aID, *aResult);
}


nsresult
LookAndFeel::GetInt(IntID aID, PRInt32* aResult)
{
  return nsLookAndFeel::GetInstance()->GetIntImpl(aID, *aResult);
}


nsresult
LookAndFeel::GetFloat(FloatID aID, float* aResult)
{
  return nsLookAndFeel::GetInstance()->GetFloatImpl(aID, *aResult);
}


PRUnichar
LookAndFeel::GetPasswordCharacter()
{
  return nsLookAndFeel::GetInstance()->GetPasswordCharacterImpl();
}


PRBool
LookAndFeel::GetEchoPassword()
{
  return nsLookAndFeel::GetInstance()->GetEchoPasswordImpl();
}


void
LookAndFeel::Refresh()
{
  nsLookAndFeel::GetInstance()->RefreshImpl();
}

} 
