



































#ifndef mozFlushType_h___
#define mozFlushType_h___





enum mozFlushType {
  Flush_Content          = 1, 
  Flush_ContentAndNotify = 2, 


  Flush_Style            = 3, 
  Flush_Frames           = Flush_Style,
  Flush_Layout           = 4, 
  Flush_Display          = 5  
};

#endif 
