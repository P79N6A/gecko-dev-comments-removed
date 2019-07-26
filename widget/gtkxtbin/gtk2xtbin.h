

 




#ifndef __GTK_XTBIN_H__
#define __GTK_XTBIN_H__

#include <gtk/gtk.h>
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

typedef struct _XtClient XtClient;

struct _XtClient {
  Display	*xtdisplay;
  Widget	top_widget;    
  Widget	child_widget;  
  Visual	*xtvisual;
  int		xtdepth;
  Colormap	xtcolormap;
  Window	oldwindow;
};

#if (GTK_MAJOR_VERSION == 2)
typedef struct _GtkXtBin GtkXtBin;
typedef struct _GtkXtBinClass GtkXtBinClass;

#define GTK_TYPE_XTBIN                  (gtk_xtbin_get_type ())
#define GTK_XTBIN(obj)                  (G_TYPE_CHECK_INSTANCE_CAST  ((obj), \
                                         GTK_TYPE_XTBIN, GtkXtBin))
#define GTK_XTBIN_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                         GTK_TYPE_XTBIN, GtkXtBinClass))
#define GTK_IS_XTBIN(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                         GTK_TYPE_XTBIN))
#define GTK_IS_XTBIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                         GTK_TYPE_XTBIN))

struct _GtkXtBin
{
  GtkSocket      gsocket;
  GdkWindow     *parent_window;
  Display       *xtdisplay;        

  Window         xtwindow;         
  XtClient	 xtclient;         
};
  
struct _GtkXtBinClass
{
  GtkSocketClass parent_class;
};

GTKXTBIN_API(GType)       gtk_xtbin_get_type (void);
GTKXTBIN_API(GtkWidget *) gtk_xtbin_new (GdkWindow *parent_window, String *f);
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


void xt_client_create(XtClient * xtclient, Window embeder, int height, int width);
void xt_client_unrealize(XtClient* xtclient);
void xt_client_destroy(XtClient* xtclient);
void xt_client_init(XtClient * xtclient, Visual *xtvisual, Colormap xtcolormap, int xtdepth);
void xt_client_xloop_create(void);
void xt_client_xloop_destroy(void);
Display * xt_client_get_display(void);

#ifdef __cplusplus
}
#endif 
#endif

