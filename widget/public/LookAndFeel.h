




































#ifndef __LookAndFeel
#define __LookAndFeel

#ifndef MOZILLA_INTERNAL_API
#error "This header is only usable from within libxul (MOZILLA_INTERNAL_API)."
#endif

#include "nsISupports.h"
#include "nsColor.h"

  
struct nsSize;


#define NS_ILOOKANDFEEL_IID \
{ 0x89401022, 0x94b3, 0x413e, \
  { 0xa6, 0xb8, 0x22, 0x03, 0xda, 0xb8, 0x24, 0xf3 } }

class nsILookAndFeel: public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILOOKANDFEEL_IID)

  
  
  typedef enum {

    
    

    eColor_WindowBackground,
    eColor_WindowForeground,
    eColor_WidgetBackground,
    eColor_WidgetForeground,
    eColor_WidgetSelectBackground,
    eColor_WidgetSelectForeground,
    eColor_Widget3DHighlight,
    eColor_Widget3DShadow,
    eColor_TextBackground,
    eColor_TextForeground,
    eColor_TextSelectBackground,
    eColor_TextSelectForeground,
    eColor_TextSelectBackgroundDisabled,
    eColor_TextSelectBackgroundAttention,
    eColor_TextHighlightBackground,
    eColor_TextHighlightForeground,

    eColor_IMERawInputBackground,
    eColor_IMERawInputForeground,
    eColor_IMERawInputUnderline,
    eColor_IMESelectedRawTextBackground,
    eColor_IMESelectedRawTextForeground,
    eColor_IMESelectedRawTextUnderline,
    eColor_IMEConvertedTextBackground,
    eColor_IMEConvertedTextForeground,
    eColor_IMEConvertedTextUnderline,
    eColor_IMESelectedConvertedTextBackground,
    eColor_IMESelectedConvertedTextForeground,
    eColor_IMESelectedConvertedTextUnderline,

    eColor_SpellCheckerUnderline,

    
    eColor_activeborder,
    eColor_activecaption,
    eColor_appworkspace,
    eColor_background,
    eColor_buttonface,
    eColor_buttonhighlight,
    eColor_buttonshadow,
    eColor_buttontext,
    eColor_captiontext,
    eColor_graytext,
    eColor_highlight,
    eColor_highlighttext,
    eColor_inactiveborder,
    eColor_inactivecaption,
    eColor_inactivecaptiontext,
    eColor_infobackground,
    eColor_infotext,
    eColor_menu,
    eColor_menutext,
    eColor_scrollbar,
    eColor_threeddarkshadow,
    eColor_threedface,
    eColor_threedhighlight,
    eColor_threedlightshadow,
    eColor_threedshadow,
    eColor_window,
    eColor_windowframe,
    eColor_windowtext,

    eColor__moz_buttondefault,
    
    eColor__moz_field,
    eColor__moz_fieldtext,
    eColor__moz_dialog,
    eColor__moz_dialogtext,
    eColor__moz_dragtargetzone,				

    eColor__moz_cellhighlight,                               
    eColor__moz_cellhighlighttext,                           
    eColor__moz_html_cellhighlight,                          
    eColor__moz_html_cellhighlighttext,                      
    eColor__moz_buttonhoverface,                             
    eColor__moz_buttonhovertext,                             
    eColor__moz_menuhover,                                   
    eColor__moz_menuhovertext,                               
    eColor__moz_menubartext,                                 
    eColor__moz_menubarhovertext,                            
    
    
    eColor__moz_eventreerow,
    eColor__moz_oddtreerow,

    
    eColor__moz_mac_chrome_active,                          
    eColor__moz_mac_chrome_inactive,                        
    eColor__moz_mac_focusring,				
    eColor__moz_mac_menuselect,				
    eColor__moz_mac_menushadow,				
    eColor__moz_mac_menutextdisable,                    
    eColor__moz_mac_menutextselect,			
    eColor__moz_mac_disabledtoolbartext,                    

    
    eColor__moz_mac_alternateprimaryhighlight, 
    eColor__moz_mac_secondaryhighlight,        

    
    eColor__moz_win_mediatext,                     
    eColor__moz_win_communicationstext,            

    
    
    
    eColor__moz_nativehyperlinktext,

    
    eColor__moz_comboboxtext,
    eColor__moz_combobox,

    
    eColor_LAST_COLOR
  } nsColorID;

  
  
  typedef enum {
    eMetric_CaretBlinkTime,                               
    eMetric_CaretWidth,                                   
    eMetric_ShowCaretDuringSelection,                       
    eMetric_SelectTextfieldsOnKeyFocus,                   
    eMetric_SubmenuDelay,                                 
    eMetric_MenusCanOverlapOSBar,                         
    eMetric_ScrollbarsCanOverlapContent,                  
    eMetric_SkipNavigatingDisabledMenuItem,               
    eMetric_DragThresholdX,                               
    eMetric_DragThresholdY,
    eMetric_UseAccessibilityTheme,                        

    eMetric_ScrollArrowStyle,                             
    eMetric_ScrollSliderStyle,                            

    eMetric_ScrollButtonLeftMouseButtonAction,            
    eMetric_ScrollButtonMiddleMouseButtonAction,          
    eMetric_ScrollButtonRightMouseButtonAction,           
 
    eMetric_TreeOpenDelay,                                
    eMetric_TreeCloseDelay,                               
    eMetric_TreeLazyScrollDelay,                          
    eMetric_TreeScrollDelay,                              
    eMetric_TreeScrollLinesMax,                           
    eMetric_TabFocusModel,                                
    eMetric_ChosenMenuItemsShouldBlink,                   

    






    eMetric_WindowsDefaultTheme,

    





    eMetric_DWMCompositor,

    






    eMetric_WindowsClassic,

    






    eMetric_TouchEnabled,

    






    eMetric_MacGraphiteTheme,

    







    eMetric_MacLionTheme,

    






    eMetric_MaemoClassic,

    














    eMetric_AlertNotificationOrigin,

    





    eMetric_ScrollToClick,

    



    eMetric_IMERawInputUnderlineStyle,
    eMetric_IMESelectedRawTextUnderlineStyle,
    eMetric_IMEConvertedTextUnderlineStyle,
    eMetric_IMESelectedConvertedTextUnderline,
    eMetric_SpellCheckerUnderlineStyle,

    


    eMetric_ImagesInMenus,
    


    eMetric_ImagesInButtons,
    


    eMetric_MenuBarDrag,
    


    eMetric_WindowsThemeIdentifier
  } nsMetricID;

  


  enum WindowsThemeIdentifier {
    eWindowsTheme_Generic = 0, 
    eWindowsTheme_Classic,
    eWindowsTheme_Aero,
    eWindowsTheme_LunaBlue,
    eWindowsTheme_LunaOlive,
    eWindowsTheme_LunaSilver,
    eWindowsTheme_Royale,
    eWindowsTheme_Zune
  };

  enum {
    eMetric_ScrollArrowNone = 0,
    eMetric_ScrollArrowStartBackward = 0x1000,
    eMetric_ScrollArrowStartForward = 0x0100,
    eMetric_ScrollArrowEndBackward = 0x0010,
    eMetric_ScrollArrowEndForward = 0x0001,
    eMetric_ScrollArrowStyleSingle =                      
      eMetric_ScrollArrowStartBackward|eMetric_ScrollArrowEndForward, 
    eMetric_ScrollArrowStyleBothAtBottom =                
      eMetric_ScrollArrowEndBackward|eMetric_ScrollArrowEndForward,
    eMetric_ScrollArrowStyleBothAtEachEnd =               
      eMetric_ScrollArrowEndBackward|eMetric_ScrollArrowEndForward|
      eMetric_ScrollArrowStartBackward|eMetric_ScrollArrowStartForward,
    eMetric_ScrollArrowStyleBothAtTop =                   
      eMetric_ScrollArrowStartBackward|eMetric_ScrollArrowStartForward
  };
  enum {
    eMetric_ScrollThumbStyleNormal,
    eMetric_ScrollThumbStyleProportional
  };
  
  
  
  typedef enum {
    eMetricFloat_IMEUnderlineRelativeSize,
    eMetricFloat_SpellCheckerUnderlineRelativeSize,

    
    
    eMetricFloat_CaretAspectRatio
  } nsMetricFloatID;

  NS_IMETHOD GetColor(const nsColorID aID, nscolor &aColor) = 0;
  NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric) = 0;
  NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric) = 0;
  virtual PRUnichar GetPasswordCharacter()
  {
    return PRUnichar('*');
  }

  virtual PRBool GetEchoPassword()
  {
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    return PR_TRUE;
#else
    return PR_FALSE;
#endif
  }

  NS_IMETHOD LookAndFeelChanged() = 0;


#ifdef NS_DEBUG
  typedef enum {
    eMetricSize_TextField = 0,
    eMetricSize_TextArea  = 1,
    eMetricSize_ListBox   = 2,
    eMetricSize_ComboBox  = 3,
    eMetricSize_Radio     = 4,
    eMetricSize_CheckBox  = 5,
    eMetricSize_Button    = 6
  } nsMetricNavWidgetID;

  typedef enum {
    eMetricSize_Courier   = 0,
    eMetricSize_SansSerif = 1
  } nsMetricNavFontID;

  
  
  
  
  NS_IMETHOD GetNavSize(const nsMetricNavWidgetID aWidgetID,
                        const nsMetricNavFontID   aFontID, 
                        const PRInt32             aFontSize, 
                        nsSize &aSize) = 0;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILookAndFeel, NS_ILOOKANDFEEL_IID)

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

    

    
    eColorID__moz_mac_chrome_active,
    
    eColorID__moz_mac_chrome_inactive,
    
    eColorID__moz_mac_focusring,
    
    eColorID__moz_mac_menuselect,
    
    eColorID__moz_mac_menushadow,
    
    eColorID__moz_mac_menutextdisable,
    
    eColorID__moz_mac_menutextselect,
    
    eColorID__moz_mac_disabledtoolbartext,

    

    
    eColorID__moz_mac_alternateprimaryhighlight,
    
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
    
    eIntID_ScrollbarsCanOverlapContent,
    
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

    






    eIntID_TouchEnabled,

    






    eIntID_MacGraphiteTheme,

    







    eIntID_MacLionTheme,

    






    eIntID_MaemoClassic,

    














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
    


    eIntID_WindowsThemeIdentifier
  };

  


  enum WindowsTheme {
    eWindowsTheme_Generic = 0, 
    eWindowsTheme_Classic,
    eWindowsTheme_Aero,
    eWindowsTheme_LunaBlue,
    eWindowsTheme_LunaOlive,
    eWindowsTheme_LunaSilver,
    eWindowsTheme_Royale,
    eWindowsTheme_Zune
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

  














  static nsresult GetColor(ColorID aID, nscolor* aResult);

  







  static nsresult GetInt(IntID aID, PRInt32* aResult);
  static nsresult GetFloat(FloatID aID, float* aResult);

  static nscolor GetColor(ColorID aID, nscolor aDefault = NS_RGB(0, 0, 0))
  {
    nscolor result;
    if (NS_FAILED(GetColor(aID, &result))) {
      return aDefault;
    }
    return result;
  }

  static PRInt32 GetInt(IntID aID, PRInt32 aDefault = 0)
  {
    PRInt32 result;
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

  



  static PRUnichar GetPasswordCharacter();

  




  static PRBool GetEchoPassword();

  



  static void Refresh();
};

} 





#define NS_DONT_CHANGE_COLOR 	NS_RGB(0x01, 0x01, 0x01)






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
