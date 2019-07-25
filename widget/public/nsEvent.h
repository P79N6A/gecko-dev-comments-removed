




































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

class nsEvent;

class nsGUIEvent;
class nsSizeEvent;
class nsSizeModeEvent;
class nsZLevelEvent;
class nsPaintEvent;
class nsScrollbarEvent;
class nsScrollPortEvent;
class nsInputEvent;
class nsMouseEvent;
class nsDragEvent;
#ifdef ACCESSIBILITY
class nsAccessibleEvent;
#endif
class nsKeyEvent;
class nsTextEvent;
class nsCompositionEvent;
class nsMouseScrollEvent;
class nsReconversionEvent;
class nsTooltipEvent;
class nsSimpleGestureEvent;
class nsContentCommandEvent;

#endif 
