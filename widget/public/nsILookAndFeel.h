




































#ifndef __nsILookAndFeel
#define __nsILookAndFeel
#include "nsISupports.h"
#include "nsColor.h"

  
struct nsSize;



#define NS_ILOOKANDFEEL_IID \
{ 0xbec234d0, 0xaaa5, 0x430d, \
    { 0x84, 0x35, 0xb1, 0x01, 0x00, 0xf7, 0x80, 0x03} }


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
    eColor__moz_buttonhoverface,                             
    eColor__moz_buttonhovertext,                             
    eColor__moz_menuhover,                                   
    eColor__moz_menuhovertext,                               
    eColor__moz_menubarhovertext,                            

    
    eColor__moz_mac_focusring,				
    eColor__moz_mac_menuselect,				
    eColor__moz_mac_menushadow,				
    eColor__moz_mac_menutextdisable,                    
    eColor__moz_mac_menutextselect,			

  	
  	eColor__moz_mac_accentlightesthighlight,
    eColor__moz_mac_accentregularhighlight,
    eColor__moz_mac_accentface,
    eColor__moz_mac_accentlightshadow,
    eColor__moz_mac_accentregularshadow,
    eColor__moz_mac_accentdarkshadow,
    eColor__moz_mac_accentdarkestshadow,
    
    
    eColor__moz_mac_alternateprimaryhighlight, 
    eColor__moz_mac_secondaryhighlight,        
  
    
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
    eMetric_DragFullWindow,                               
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

    














    eMetric_AlertNotificationOrigin
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
    eMetricFloat_IMEUnderlineRelativeSize
  } nsMetricFloatID;

  NS_IMETHOD GetColor(const nsColorID aID, nscolor &aColor) = 0;
  NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric) = 0;
  NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric) = 0;
  virtual PRUnichar GetPasswordCharacter()
  {
    return PRUnichar('*');
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

#define NS_IS_IME_SPECIAL_COLOR(c) ((c) == NS_TRANSPARENT || \
                                    (c) == NS_SAME_AS_FOREGROUND_COLOR || \
                                    (c) == NS_40PERCENT_FOREGROUND_COLOR)





#define NS_ALERT_HORIZONTAL 1
#define NS_ALERT_LEFT       2
#define NS_ALERT_TOP        4

#endif 
