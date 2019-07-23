







































#ifndef __XLIB_XTBIN_H__
#define __XLIB_XTBIN_H__

#include "xlibrgb.h"

#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/X.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XtTMRec {
    XtTranslations  translations;       
    XtBoundActions  proc_table;         
    struct _XtStateRec *current_state;  
    unsigned long   lastEventTime;
} XtTMRec, *XtTM;   

typedef struct _CorePart {
    Widget          self;               
    WidgetClass     widget_class;       
    Widget          parent;             
    XrmName         xrm_name;           
    Boolean         being_destroyed;    
    XtCallbackList  destroy_callbacks;  
    XtPointer       constraints;        
    Position        x, y;               
    Dimension       width, height;      
    Dimension       border_width;       
    Boolean         managed;            
    Boolean         sensitive;          
    Boolean         ancestor_sensitive; 
    XtEventTable    event_table;        
    XtTMRec         tm;                 
    XtTranslations  accelerators;       
    Pixel           border_pixel;       
    Pixmap          border_pixmap;      
    WidgetList      popup_list;         
    Cardinal        num_popups;         
    String          name;               
    Screen          *screen;            
    Colormap        colormap;           
    Window          window;             
    Cardinal        depth;              
    Pixel           background_pixel;   
    Pixmap          background_pixmap;  
    Boolean         visible;            
    Boolean         mapped_when_managed;
} CorePart;

typedef struct _WidgetRec {
    CorePart    core;
 } WidgetRec, CoreRec;   
#ifdef __cplusplus
}
#endif 

class xtbin {
public:
  xtbin();
  ~xtbin();

  void xtbin_init();
  void xtbin_realize();
  void xtbin_new(Window aParent);
  void xtbin_destroy();
  void xtbin_resize(int aX, int aY, int aWidth, int aHeight);
  Window xtbin_xtwindow() { return xtwindow; }
  int xtbin_initialized();
  void sync();
  
private: 
  int initialized;
  XlibRgbHandle *mXlibRgbHandle;
  Display *xtdisplay;
  Window xtwindow;
  XtAppContext app_context;
  Window parent_window;
  Widget xtwidget;
  Window window;
  Window oldwindow;
  int x, y, width, height;
};


#endif

