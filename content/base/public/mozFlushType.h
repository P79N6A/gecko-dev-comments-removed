



































#ifndef mozFlushType_h___
#define mozFlushType_h___





enum mozFlushType {
  Flush_Content           = 0x1,   
  Flush_SinkNotifications = 0x2,   
  Flush_StyleReresolves   = 0x4,   
  Flush_OnlyReflow        = 0x8,   
  Flush_OnlyPaint         = 0x10,  
  Flush_ContentAndNotify  = (Flush_Content | Flush_SinkNotifications),
  Flush_Frames            = (Flush_Content | Flush_SinkNotifications |
                             Flush_StyleReresolves),
  Flush_Style             = (Flush_Content | Flush_SinkNotifications |
                             Flush_StyleReresolves),
  Flush_Layout            = (Flush_Content | Flush_SinkNotifications |
                             Flush_StyleReresolves | Flush_OnlyReflow),
  Flush_Display           = (Flush_Content | Flush_SinkNotifications |
                             Flush_StyleReresolves | Flush_OnlyReflow |
                             Flush_OnlyPaint)
};

#endif 
