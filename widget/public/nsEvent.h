




































#ifndef nsEvent_h__
#define nsEvent_h__











enum nsEventStatus {  
    
  nsEventStatus_eIgnore,            
    
  nsEventStatus_eConsumeNoDefault, 
    
  nsEventStatus_eConsumeDoDefault  
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
class nsAccessibleEvent;
class nsKeyEvent;
class nsTextEvent;
class nsCompositionEvent;
class nsMouseScrollEvent;
class nsReconversionEvent;
class nsTooltipEvent;
class nsMenuEvent;

struct nsTextEventReply;
struct nsReconversionEventReply;
struct nsQueryCaretRectEventReply;

#endif 
