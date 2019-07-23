

 


































   

#ifndef __GTK_XTBIN_H__
#define __GTK_XTBIN_H__

#include <gtk/gtkwidget.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#ifdef MOZILLA_CLIENT
#include "nscore.h"
#ifdef _IMPL_GTKXTBIN_API
#define GTKXTBIN_API(type) NS_EXPORT_(type)
#else
#define GTKXTBIN_API(type) NS_IMPORT_(type)
#endif
#else
#define GTKXTBIN_API(type) type
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GtkXtBin GtkXtBin;
typedef struct _GtkXtBinClass GtkXtBinClass;

#define GTK_TYPE_XTBIN                  (gtk_xtbin_get_type ())
#define GTK_XTBIN(obj)                  (GTK_CHECK_CAST ((obj), \
                                         GTK_TYPE_XTBIN, GtkXtBin))
#define GTK_XTBIN_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), \
                                         GTK_TYPE_XTBIN, GtkXtBinClass))
#define GTK_IS_XTBIN(obj)               (GTK_CHECK_TYPE ((obj), \
                                         GTK_TYPE_XTBIN))
#define GTK_IS_XTBIN_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), \
                                         GTK_TYPE_XTBIN))

struct _GtkXtBin
{
  GtkWidget      widget;
  GdkWindow     *parent_window;
  Display       *xtdisplay;        

  Widget         xtwidget;         

  Window         xtwindow;         
  gint           x, y;
  gint           width, height;
};
  
struct _GtkXtBinClass
{
  GtkWidgetClass widget_class;
};

GTKXTBIN_API(GtkType)    gtk_xtbin_get_type (void);
GTKXTBIN_API(GtkWidget *) gtk_xtbin_new (GdkWindow *parent_window, String *f);
GTKXTBIN_API(void)       gtk_xtbin_set_position (GtkXtBin *xtbin,
                                                 gint       x,
                                                 gint       y);
GTKXTBIN_API(void)       gtk_xtbin_resize (GtkWidget *widget,
                                           gint       width,
                                           gint       height);

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
#endif

