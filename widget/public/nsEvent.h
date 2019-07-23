




































#ifndef nsEvent_h__
#define nsEvent_h__











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
class nsAccessibleEvent;
class nsKeyEvent;
class nsTextEvent;
class nsCompositionEvent;
class nsMouseScrollEvent;
class nsReconversionEvent;
class nsTooltipEvent;
class nsMenuEvent;
class nsSimpleGestureEvent;
class nsContentCommandEvent;

struct nsTextEventReply;

#endif 
