




































#ifndef nsEvent_h__
#define nsEvent_h__







enum UIStateChangeType {
  UIStateChangeType_NoChange,
  UIStateChangeType_Set,
  UIStateChangeType_Clear
};





enum nsEventStatus {  
    
  nsEventStatus_eIgnore,            
    
  nsEventStatus_eConsumeNoDefault, 
    
  nsEventStatus_eConsumeDoDefault  
};




enum nsSizeMode {
  nsSizeMode_Normal = 0,
  nsSizeMode_Minimized,
  nsSizeMode_Maximized,
  nsSizeMode_Fullscreen
};

struct nsAlternativeCharCode;
struct nsTextRangeStyle;
struct nsTextRange;

class nsEvent;
class nsGUIEvent;
class nsScriptErrorEvent;
class nsSizeEvent;
class nsSizeModeEvent;
class nsZLevelEvent;
class nsPaintEvent;
class nsScrollbarEvent;
class nsScrollPortEvent;
class nsScrollAreaEvent;
class nsInputEvent;
class nsMouseEvent_base;
class nsMouseEvent;
class nsDragEvent;
#ifdef ACCESSIBILITY
class nsAccessibleEvent;
#endif
class nsKeyEvent;
class nsTextEvent;
class nsCompositionEvent;
class nsMouseScrollEvent;
class nsGestureNotifyEvent;
class nsQueryContentEvent;
class nsFocusEvent;
class nsSelectionEvent;
class nsContentCommandEvent;
class nsMozTouchEvent;
class nsTouchEvent;
class nsFormEvent;
class nsCommandEvent;
class nsUIEvent;
class nsSimpleGestureEvent;
class nsTransitionEvent;
class nsAnimationEvent;
class nsUIStateChangeEvent;
class nsPluginEvent;

#endif 
