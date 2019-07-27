




#ifndef __LookAndFeel
#define __LookAndFeel

#ifndef MOZILLA_INTERNAL_API
#error "This header is only usable from within libxul (MOZILLA_INTERNAL_API)."
#endif

#include "nsDebug.h"
#include "nsColor.h"

struct gfxFontStyle;

namespace mozilla {

class LookAndFeel
{
public:
  
  
  enum ColorID {

    
    

    eColorID_WindowBackground,
    eColorID_WindowForeground,
    eColorID_WidgetBackground,
    eColorID_WidgetForeground,
    eColorID_WidgetSelectBackground,
    eColorID_WidgetSelectForeground,
    eColorID_Widget3DHighlight,
    eColorID_Widget3DShadow,
    eColorID_TextBackground,
    eColorID_TextForeground,
    eColorID_TextSelectBackground,
    eColorID_TextSelectForeground,
    eColorID_TextSelectForegroundCustom,
    eColorID_TextSelectBackgroundDisabled,
    eColorID_TextSelectBackgroundAttention,
    eColorID_TextHighlightBackground,
    eColorID_TextHighlightForeground,

    eColorID_IMERawInputBackground,
    eColorID_IMERawInputForeground,
    eColorID_IMERawInputUnderline,
    eColorID_IMESelectedRawTextBackground,
    eColorID_IMESelectedRawTextForeground,
    eColorID_IMESelectedRawTextUnderline,
    eColorID_IMEConvertedTextBackground,
    eColorID_IMEConvertedTextForeground,
    eColorID_IMEConvertedTextUnderline,
    eColorID_IMESelectedConvertedTextBackground,
    eColorID_IMESelectedConvertedTextForeground,
    eColorID_IMESelectedConvertedTextUnderline,

    eColorID_SpellCheckerUnderline,

    
    eColorID_activeborder,
    eColorID_activecaption,
    eColorID_appworkspace,
    eColorID_background,
    eColorID_buttonface,
    eColorID_buttonhighlight,
    eColorID_buttonshadow,
    eColorID_buttontext,
    eColorID_captiontext,
    eColorID_graytext,
    eColorID_highlight,
    eColorID_highlighttext,
    eColorID_inactiveborder,
    eColorID_inactivecaption,
    eColorID_inactivecaptiontext,
    eColorID_infobackground,
    eColorID_infotext,
    eColorID_menu,
    eColorID_menutext,
    eColorID_scrollbar,
    eColorID_threeddarkshadow,
    eColorID_threedface,
    eColorID_threedhighlight,
    eColorID_threedlightshadow,
    eColorID_threedshadow,
    eColorID_window,
    eColorID_windowframe,
    eColorID_windowtext,

    eColorID__moz_buttondefault,
    
    eColorID__moz_field,
    eColorID__moz_fieldtext,
    eColorID__moz_dialog,
    eColorID__moz_dialogtext,
    
    eColorID__moz_dragtargetzone,

    
    eColorID__moz_cellhighlight,
    
    eColorID__moz_cellhighlighttext,
    
    eColorID__moz_html_cellhighlight,
    
    eColorID__moz_html_cellhighlighttext,
    
    eColorID__moz_buttonhoverface,
    
    eColorID__moz_buttonhovertext,
    
    eColorID__moz_menuhover,
    
    eColorID__moz_menuhovertext,
    
    eColorID__moz_menubartext,
    
    eColorID__moz_menubarhovertext,
    
    
    eColorID__moz_eventreerow,
    eColorID__moz_oddtreerow,

    

    
    eColorID__moz_mac_buttonactivetext,
    
    eColorID__moz_mac_chrome_active,
    
    eColorID__moz_mac_chrome_inactive,
    
    eColorID__moz_mac_defaultbuttontext,
    
    eColorID__moz_mac_focusring,
    
    eColorID__moz_mac_menuselect,
    
    eColorID__moz_mac_menushadow,
    
    eColorID__moz_mac_menutextdisable,
    
    eColorID__moz_mac_menutextselect,
    
    eColorID__moz_mac_disabledtoolbartext,
    
    eColorID__moz_mac_secondaryhighlight,

    

    
    eColorID__moz_win_mediatext,
    
    eColorID__moz_win_communicationstext,

    
    
    
    
    
    eColorID__moz_nativehyperlinktext,

    
    eColorID__moz_comboboxtext,
    eColorID__moz_combobox,

    
    eColorID_LAST_COLOR
  };

  
  
  enum IntID {
    
    eIntID_CaretBlinkTime,
    
    eIntID_CaretWidth,
    
    eIntID_ShowCaretDuringSelection,
    
    eIntID_SelectTextfieldsOnKeyFocus,
    
    eIntID_SubmenuDelay,
    
    eIntID_MenusCanOverlapOSBar,
    
    eIntID_UseOverlayScrollbars,
    
    eIntID_AllowOverlayScrollbarsOverlap,
    
    eIntID_ShowHideScrollbars,
    
    eIntID_SkipNavigatingDisabledMenuItem,
    
    
    eIntID_DragThresholdX,
    eIntID_DragThresholdY,
    
    eIntID_UseAccessibilityTheme,

    
    eIntID_ScrollArrowStyle,
    
    eIntID_ScrollSliderStyle,

    
    eIntID_ScrollButtonLeftMouseButtonAction,
    
    eIntID_ScrollButtonMiddleMouseButtonAction,
    
    eIntID_ScrollButtonRightMouseButtonAction,

    
    eIntID_TreeOpenDelay,
    
    eIntID_TreeCloseDelay,
    
    eIntID_TreeLazyScrollDelay,
    
    eIntID_TreeScrollDelay,
    
    eIntID_TreeScrollLinesMax,
    
    eIntID_TabFocusModel,
    
    eIntID_ChosenMenuItemsShouldBlink,

    






    eIntID_WindowsDefaultTheme,

    





    eIntID_DWMCompositor,

    






    eIntID_WindowsClassic,

    






    eIntID_WindowsGlass,

    






    eIntID_TouchEnabled,

    






    eIntID_MacGraphiteTheme,

    







    eIntID_MacLionTheme,

   







   eIntID_MacYosemiteTheme,

    














    eIntID_AlertNotificationOrigin,

    





    eIntID_ScrollToClick,

    



    eIntID_IMERawInputUnderlineStyle,
    eIntID_IMESelectedRawTextUnderlineStyle,
    eIntID_IMEConvertedTextUnderlineStyle,
    eIntID_IMESelectedConvertedTextUnderline,
    eIntID_SpellCheckerUnderlineStyle,

    


    eIntID_ImagesInMenus,
    


    eIntID_ImagesInButtons,
    


    eIntID_MenuBarDrag,
    


    eIntID_WindowsThemeIdentifier,
    


    eIntID_OperatingSystemVersionIdentifier,
    



    eIntID_ScrollbarButtonAutoRepeatBehavior,
    


    eIntID_TooltipDelay,
    



    eIntID_SwipeAnimationEnabled,

    






    eIntID_ColorPickerAvailable,

    




     eIntID_PhysicalHomeButton,
 
     



     eIntID_ScrollbarDisplayOnMouseMove,
 
     


     eIntID_ScrollbarFadeBeginDelay,
     eIntID_ScrollbarFadeDuration
  };

  


  enum WindowsTheme {
    eWindowsTheme_Generic = 0, 
    eWindowsTheme_Classic,
    eWindowsTheme_Aero,
    eWindowsTheme_LunaBlue,
    eWindowsTheme_LunaOlive,
    eWindowsTheme_LunaSilver,
    eWindowsTheme_Royale,
    eWindowsTheme_Zune,
    eWindowsTheme_AeroLite
  };

  


  enum OperatingSystemVersion {
    eOperatingSystemVersion_WindowsXP = 0,
    eOperatingSystemVersion_WindowsVista,
    eOperatingSystemVersion_Windows7,
    eOperatingSystemVersion_Windows8,
    eOperatingSystemVersion_Unknown
  };

  enum {
    eScrollArrow_None = 0,
    eScrollArrow_StartBackward = 0x1000,
    eScrollArrow_StartForward = 0x0100,
    eScrollArrow_EndBackward = 0x0010,
    eScrollArrow_EndForward = 0x0001
  };

  enum {
    
    eScrollArrowStyle_Single =
      eScrollArrow_StartBackward | eScrollArrow_EndForward, 
    
    eScrollArrowStyle_BothAtBottom =
      eScrollArrow_EndBackward | eScrollArrow_EndForward,
    
    eScrollArrowStyle_BothAtEachEnd =
      eScrollArrow_EndBackward | eScrollArrow_EndForward |
      eScrollArrow_StartBackward | eScrollArrow_StartForward,
    
    eScrollArrowStyle_BothAtTop =
      eScrollArrow_StartBackward | eScrollArrow_StartForward
  };

  enum {
    eScrollThumbStyle_Normal,
    eScrollThumbStyle_Proportional
  };

  
  
  enum FloatID {
    eFloatID_IMEUnderlineRelativeSize,
    eFloatID_SpellCheckerUnderlineRelativeSize,

    
    
    eFloatID_CaretAspectRatio
  };

  
  
  enum FontID {
    eFont_Caption = 1,     
    eFont_Icon,
    eFont_Menu,
    eFont_MessageBox,
    eFont_SmallCaption,
    eFont_StatusBar,

    eFont_Window,          
    eFont_Document,
    eFont_Workspace,
    eFont_Desktop,
    eFont_Info,
    eFont_Dialog,
    eFont_Button,
    eFont_PullDownMenu,
    eFont_List,
    eFont_Field,

    eFont_Tooltips,        
    eFont_Widget
  };

  














  static nsresult GetColor(ColorID aID, nscolor* aResult);

  







  static nsresult GetInt(IntID aID, int32_t* aResult);
  static nsresult GetFloat(FloatID aID, float* aResult);

  static nscolor GetColor(ColorID aID, nscolor aDefault = NS_RGB(0, 0, 0))
  {
    nscolor result = NS_RGB(0, 0, 0);
    if (NS_FAILED(GetColor(aID, &result))) {
      return aDefault;
    }
    return result;
  }

  static int32_t GetInt(IntID aID, int32_t aDefault = 0)
  {
    int32_t result;
    if (NS_FAILED(GetInt(aID, &result))) {
      return aDefault;
    }
    return result;
  }

  static float GetFloat(FloatID aID, float aDefault = 0.0f)
  {
    float result;
    if (NS_FAILED(GetFloat(aID, &result))) {
      return aDefault;
    }
    return result;
  }

  









  static bool GetFont(FontID aID, nsString& aName, gfxFontStyle& aStyle,
                      float aDevPixPerCSSPixel);

  



  static char16_t GetPasswordCharacter();

  




  static bool GetEchoPassword();

  



  static uint32_t GetPasswordMaskDelay();

  



  static void Refresh();
};

} 





#define NS_DONT_CHANGE_COLOR 	NS_RGB(0x01, 0x01, 0x01)





#define NS_CHANGE_COLOR_IF_SAME_AS_BG NS_RGB(0x02, 0x02, 0x02)






#define NS_TRANSPARENT                NS_RGBA(0x01, 0x00, 0x00, 0x00)

#define NS_SAME_AS_FOREGROUND_COLOR   NS_RGBA(0x02, 0x00, 0x00, 0x00)
#define NS_40PERCENT_FOREGROUND_COLOR NS_RGBA(0x03, 0x00, 0x00, 0x00)

#define NS_IS_SELECTION_SPECIAL_COLOR(c) ((c) == NS_TRANSPARENT || \
                                          (c) == NS_SAME_AS_FOREGROUND_COLOR || \
                                          (c) == NS_40PERCENT_FOREGROUND_COLOR)





#define NS_ALERT_HORIZONTAL 1
#define NS_ALERT_LEFT       2
#define NS_ALERT_TOP        4

#endif 
