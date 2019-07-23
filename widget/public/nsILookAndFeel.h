




































#ifndef __nsILookAndFeel
#define __nsILookAndFeel
#include "nsISupports.h"
#include "nsColor.h"

  
struct nsSize;



#define NS_ILOOKANDFEEL_IID \
{ 0xc23ca876, 0x6ecf, 0x49c6, \
    { 0xb2, 0xb4, 0x5b, 0xe5, 0x16, 0xb5, 0x0e, 0x28 } }

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
    eMetric_WindowTitleHeight,
    eMetric_WindowBorderWidth,
    eMetric_WindowBorderHeight,
    eMetric_Widget3DBorder,
    eMetric_TextFieldBorder,                              
    eMetric_TextFieldHeight,
    eMetric_TextVerticalInsidePadding,                    
    eMetric_TextShouldUseVerticalInsidePadding,           
    eMetric_TextHorizontalInsideMinimumPadding,  
    eMetric_TextShouldUseHorizontalInsideMinimumPadding,  
    eMetric_ButtonHorizontalInsidePaddingNavQuirks,  
    eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks, 
    eMetric_CheckboxSize,
    eMetric_RadioboxSize,
    
    eMetric_ListShouldUseHorizontalInsideMinimumPadding,  
    eMetric_ListHorizontalInsideMinimumPadding,         

    eMetric_ListShouldUseVerticalInsidePadding,           
    eMetric_ListVerticalInsidePadding,                    

    eMetric_CaretBlinkTime,                               
    eMetric_CaretWidth,                                   
    eMetric_ShowCaretDuringSelection,                       
    eMetric_SelectTextfieldsOnKeyFocus,                   
    eMetric_SubmenuDelay,                                 
    eMetric_MenusCanOverlapOSBar,                         
    eMetric_SkipNavigatingDisabledMenuItem,               
    eMetric_DragThresholdX,                               
    eMetric_DragThresholdY,
    eMetric_UseAccessibilityTheme,                        
    eMetric_IsScreenReaderActive,                         

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

    






    eMetric_WindowsDefaultTheme,

    





    eMetric_DWMCompositor,

    






    eMetric_WindowsClassic,

    






    eMetric_TouchEnabled,

    






    eMetric_MacGraphiteTheme,

    






    eMetric_MaemoClassic,

    














    eMetric_AlertNotificationOrigin,

    





    eMetric_ScrollToClick,

    



    eMetric_IMERawInputUnderlineStyle,
    eMetric_IMESelectedRawTextUnderlineStyle,
    eMetric_IMEConvertedTextUnderlineStyle,
    eMetric_IMESelectedConvertedTextUnderline,
    eMetric_SpellCheckerUnderlineStyle,

    


    eMetric_ImagesInMenus,
    


    eMetric_ImagesInButtons
  } nsMetricID;

  enum {
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
    eMetricFloat_TextFieldVerticalInsidePadding,
    eMetricFloat_TextFieldHorizontalInsidePadding,
    eMetricFloat_TextAreaVerticalInsidePadding,
    eMetricFloat_TextAreaHorizontalInsidePadding,
    eMetricFloat_ListVerticalInsidePadding,
    eMetricFloat_ListHorizontalInsidePadding,
    eMetricFloat_ButtonVerticalInsidePadding,
    eMetricFloat_ButtonHorizontalInsidePadding,
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


	
	
	
	
#define NS_DONT_CHANGE_COLOR 	NS_RGB(0x01, 0x01, 0x01)






#define NS_TRANSPARENT                NS_RGBA(0x01, 0x00, 0x00, 0x00)

#define NS_SAME_AS_FOREGROUND_COLOR   NS_RGBA(0x02, 0x00, 0x00, 0x00)
#define NS_40PERCENT_FOREGROUND_COLOR NS_RGBA(0x03, 0x00, 0x00, 0x00)

#define NS_IS_SELECTION_SPECIAL_COLOR(c) ((c) == NS_TRANSPARENT || \
                                          (c) == NS_SAME_AS_FOREGROUND_COLOR || \
                                          (c) == NS_40PERCENT_FOREGROUND_COLOR)





#define NS_UNDERLINE_STYLE_NONE   0
#define NS_UNDERLINE_STYLE_DOTTED 1
#define NS_UNDERLINE_STYLE_DASHED 2
#define NS_UNDERLINE_STYLE_SOLID  3
#define NS_UNDERLINE_STYLE_DOUBLE 4
#define NS_UNDERLINE_STYLE_WAVY   5

#define NS_IS_VALID_UNDERLINE_STYLE(s) \
  (NS_UNDERLINE_STYLE_NONE <= (s) && (s) <= NS_UNDERLINE_STYLE_WAVY)





#define NS_ALERT_HORIZONTAL 1
#define NS_ALERT_LEFT       2
#define NS_ALERT_TOP        4

#endif 
