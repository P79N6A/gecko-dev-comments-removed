

















































#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>


#ifdef MOZ_X11
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#endif

#include "npapi.h"
#include "nullplugin.h"
#include "prprf.h"

#define DIALOGID "dialog"


static MimeTypeElement *head = NULL;


void
destroyWidget(PluginInstance *This)
{
    if (This && This->dialogBox)
    {
      gtk_widget_destroy (GTK_WIDGET(This->dialogBox));
    }
}


static void 
DialogOKClicked (GtkButton *button, gpointer data)
{
    PluginInstance* This = (PluginInstance*) data;
    GtkWidget* dialogWindow = gtk_object_get_data(GTK_OBJECT(button), DIALOGID);
    char *url;

    gtk_object_remove_data(GTK_OBJECT(button), DIALOGID);

    if (This->pluginsFileUrl != NULL)
    {
        
        static const char buf[] = 
          "javascript:netscape.softupdate.Trigger.StartSoftwareUpdate(\"%s\")";

        url = NPN_MemAlloc(strlen(This->pluginsFileUrl) + (sizeof(buf) - 2));
        if (url != NULL)
        {
            
            sprintf(url, buf, This->pluginsFileUrl);
            NPN_GetURL(This->instance, url, TARGET);
            NPN_MemFree(url);
        }
    }
    else
    {
        
        char* address = This->pluginsPageUrl;
        if (address == NULL || *address == 0)
        {
            address = PLUGINSPAGE_URL;
        }

        url = NPN_MemAlloc(strlen(address) + 1 + strlen(This->type)+1);
        if (url != NULL)
        {
            NPN_PushPopupsEnabledState(This->instance, TRUE);
                
            sprintf(url, "%s?%s", address, This->type);
            if (strcmp (This->type, JVM_MINETYPE) == 0) 
            {
                NPN_GetURL(This->instance, JVM_SMARTUPDATE_URL , TARGET);
            }
            else 
            {
                NPN_GetURL(This->instance, url, TARGET);
            }
            NPN_MemFree(url);
            NPN_PopPopupsEnabledState(This->instance);
        }
    }
    destroyWidget(This);
}


static void 
DialogCancelClicked (GtkButton *button, gpointer data) 
{
    destroyWidget((PluginInstance*) data);
}


static gboolean
DialogEscapePressed (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_Escape)
    {
        gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
        gtk_object_destroy (GTK_OBJECT (widget));
        return TRUE;
    }

    return FALSE;
}


static GtkWidget *
AddWidget (GtkWidget *widget, GtkWidget *packingbox)
{
    gtk_box_pack_start(GTK_BOX(packingbox), widget, TRUE, TRUE, 2);
    return widget;
}





static gboolean
isEqual(NPMIMEType t1, NPMIMEType t2)
{
    return (t1 && t2) ? (strcmp(t1, t2) == 0) : FALSE; 
}

static MimeTypeElement * 
isExist(MimeTypeElement **typelist, NPMIMEType type)
{
    MimeTypeElement *ele;

    ele = *typelist;
    while (ele != NULL) {
        if (isEqual(ele->pinst->type, type))
            return ele;
        ele = ele->next;
    }
    return NULL;
}

NPMIMEType
dupMimeType(NPMIMEType type)
{
    NPMIMEType mimetype = NPN_MemAlloc(strlen(type)+1);
    if (mimetype)
        strcpy(mimetype, type);
    return(mimetype);
}

static gboolean 
addToList(MimeTypeElement **typelist, PluginInstance *This)
{
    if (This && This->type && !isExist(typelist, This->type))
    {
        MimeTypeElement *ele;
        if ((ele = (MimeTypeElement *) NPN_MemAlloc(sizeof(MimeTypeElement))))
        {
            ele->pinst = This;
            ele->next = *typelist;
            *typelist = ele;
            return(TRUE);
        }
    }
    return(FALSE);
}

static gboolean
delFromList(MimeTypeElement **typelist, PluginInstance *This)
{
    if (typelist && This)
    {
        MimeTypeElement *ele_prev;
        MimeTypeElement *ele = *typelist;
        while (ele)
        {
            if (isEqual(ele->pinst->type, This->type))
            {
                if (*typelist == ele)
                {
                    *typelist = ele->next;
                } else {
                    ele_prev->next = ele->next;
                }
                NPN_MemFree(ele);
                return(TRUE);
            }
            ele_prev = ele;
            ele = ele->next;
        }
    }
    return(FALSE);
}

static void
onDestroyWidget(GtkWidget *w, gpointer data)
{
    PluginInstance* This = (PluginInstance*) data;
    if (This && This->dialogBox && This->dialogBox == w)
    {
        This->dialogBox = 0;
        delFromList(&head, This);
    }
}


void 
makeWidget(PluginInstance *This)
{
    GtkWidget *dialogWindow;
    GtkWidget *dialogMessage;
    GtkWidget *okButton;
    GtkWidget *cancelButton;
    char message[1024];
    MimeTypeElement *ele;

    if (!This) return;

    



    if ((ele = isExist(&head, This->type)))
    {
        if (ele->pinst && ele->pinst->dialogBox)
        {
            GtkWidget *top_window = gtk_widget_get_toplevel(ele->pinst->dialogBox);
            if (top_window && GTK_WIDGET_VISIBLE(top_window))
            {   
                gdk_window_show(top_window->window);
            }
        }
        return;
    }

    dialogWindow = gtk_dialog_new();
    This->dialogBox = dialogWindow;
    This->exists = TRUE;
    This->dialogBox = dialogWindow;
    addToList(&head, This);
    gtk_window_set_title(GTK_WINDOW(dialogWindow), PLUGIN_NAME);
    gtk_window_set_position(GTK_WINDOW(dialogWindow), GTK_WIN_POS_CENTER);
    gtk_window_set_modal(GTK_WINDOW(dialogWindow), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialogWindow), 20);
    gtk_window_set_policy(GTK_WINDOW(dialogWindow), FALSE, FALSE, TRUE);

    PR_snprintf(message, sizeof(message) - 1, MESSAGE, This->type);
    dialogMessage = AddWidget(gtk_label_new (message), 
                   GTK_DIALOG(dialogWindow)->vbox);

    okButton= AddWidget(gtk_button_new_with_label (OK_BUTTON), 
                   GTK_DIALOG(dialogWindow)->action_area);
    gtk_object_set_data(GTK_OBJECT(okButton), DIALOGID, dialogWindow);

    GTK_WIDGET_SET_FLAGS (okButton, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(okButton);

    cancelButton= AddWidget(gtk_button_new_with_label (CANCEL_BUTTON), 
                   GTK_DIALOG(dialogWindow)->action_area);

    gtk_signal_connect (GTK_OBJECT(okButton),  "clicked",
                        GTK_SIGNAL_FUNC(DialogOKClicked), This);

    gtk_signal_connect (GTK_OBJECT(cancelButton),  "clicked",
                        GTK_SIGNAL_FUNC(DialogCancelClicked), This);

    gtk_signal_connect(GTK_OBJECT(dialogWindow), "key_press_event",
                        GTK_SIGNAL_FUNC (DialogEscapePressed), NULL);

    
    gtk_signal_connect(GTK_OBJECT(dialogWindow), "destroy",
                        GTK_SIGNAL_FUNC(onDestroyWidget), This);

    gtk_widget_show_all(dialogWindow);
}


static char * npnul320_xpm[] = {
"32 32 6 1",
"       c None",
".      c #808080",
"+      c #F8F8F8",
"@      c #C0C0C0",
"#      c #000000",
"$      c #00F8F8",
"........................++++++++",
".++++++++++++++++++++++..+++++++",
".+++++++++++++++++++++@.@.++++++",
".++@@@@@@@@@@@@@@@@@@@@.+@.+++++",
".++@@@@@@@@@.....@@@@@@.++@.++++",
".++@@@@@@@@.+++++#@@@@@.+++@.+++",
".++@@@@@@@.++$$$$$#@@@@.++++@.++",
".++@@@@@@@.+$$$$$$#.@@@.+++++@.+",
".++@@@...@@.+$$$$#..###.#######+",
".++@@.+++$$++$$$$$##++$#......#+",
".++@@.+$$$++$$$$$$$+$$$#......#+",
".++@@.+$$$$$$$$$$$$$$$$#..@@++#+",
".++@@@$$$$$$$$$$$$$$$$#...@@++#+",
".++@@@$#$##.$$$$$$##$$#...@@++#+",
".++@@@@##...$$$$$#..##...@@@++#+",
".++@@@@@....+$$$$#.......@@@++#+",
".++@@@@@@...+$$$$#.@@@..@@@@++#+",
".++@@@@..@@.+$$$$#.@##@@@@@@++#+",
".++@@@.++$$++$$$$$##$$#@@@@@++#+",
".++@@@.+$$++$$$$$$$$$$#@@@@@++#+",
".++@@.++$$$$$$$$$$$$$$$#@@@@++#+",
".++@@.+$$$$$$$$$$$$$$$$#.@@@++#+",
".++@@.+$$##$$$$$$$##$$$#..@@++#+",
".++@@@###...$$$$$#.@###...@@++#+",
".++@@@@....$$$$$$$#.@.....@@++#+",
".++@@@@@...$$$$$$$#..@...@@@++#+",
".++@@@@@@@@#$$$$$#...@@@@@@@++#+",
".++@@@@@@@@@#####...@@@@@@@@++#+",
".++@@@@@@@@@@......@@@@@@@@@++#+",
".+++++++++++++....++++++++++++#+",
".+++++++++++++++++++++++++++++#+",
"###############################+"};


static GdkPixmap *nullPluginGdkPixmap = 0;

static GdkWindow *getGdkWindow(PluginInstance *This)
{
#ifdef MOZ_X11
    GdkWindow *gdk_window;
    Window xwin = (Window) This->window;
    Widget xt_w = XtWindowToWidget(This->display, xwin);

    if (xt_w) {
      xt_w = XtParent(xt_w);
      if (xt_w) {
         xwin = XtWindow(xt_w);
         
      }
    }
    gdk_window = gdk_window_lookup(xwin);
    return gdk_window;
#else
    return NULL;
#endif
}

static void
createPixmap(PluginInstance *This)
{
    int err = 0;

    if (nullPluginGdkPixmap == 0)
    { 
       GtkStyle *style;
       GdkBitmap *mask;
       GdkWindow *gdk_window = getGdkWindow(This);
       if (gdk_window)
       {
           GtkWidget *widget;
#ifndef MOZ_WIDGET_GTK2
           widget = (GtkWidget *)gdk_window->user_data;
#else
           gpointer user_data = NULL;
           gdk_window_get_user_data( gdk_window, &user_data);
           widget = GTK_WIDGET(user_data);
#endif
           style = gtk_widget_get_style(widget);
           nullPluginGdkPixmap = gdk_pixmap_create_from_xpm_d(gdk_window , &mask,
                                             &style->bg[GTK_STATE_NORMAL], npnul320_xpm);
#ifdef MOZ_X11
	   
	   XSync(GDK_DISPLAY(), False);
#endif
       }
    }
}

static void
drawPixmap(PluginInstance *This)
{
    if (nullPluginGdkPixmap)
    {
        int pixmap_with, pixmap_height, dest_x, dest_y;
        gdk_window_get_size((GdkWindow *)nullPluginGdkPixmap, &pixmap_with, &pixmap_height);
        dest_x = This->width/2 - pixmap_with/2;
        dest_y = This->height/2 - pixmap_height/2;
        if (dest_x >= 0 && dest_y >= 0)
        {
#ifdef MOZ_X11
            GC gc;
            gc = XCreateGC(This->display, This->window, 0, NULL);
            XCopyArea(This->display, GDK_WINDOW_XWINDOW(nullPluginGdkPixmap) , This->window, gc,
                0, 0, pixmap_with, pixmap_height, dest_x, dest_y);
            XFreeGC(This->display, gc);
#endif
        }
    }
}

static void
setCursor (PluginInstance *This)
{
#ifdef MOZ_X11
    static Cursor nullPluginCursor = 0;
    if (!nullPluginCursor)
    {
        nullPluginCursor = XCreateFontCursor(This->display, XC_hand2);
    }
    if (nullPluginCursor)
    {
        XDefineCursor(This->display, This->window, nullPluginCursor);
    }
#endif
}

#ifdef MOZ_X11
static void
xt_event_handler(Widget xt_w, PluginInstance *This, XEvent *xevent, Boolean *b)
{
    switch (xevent->type)
    {
        case Expose:
            
            while(XCheckTypedWindowEvent(This->display, This->window, Expose, xevent));
            drawPixmap(This);
            break;
        case ButtonRelease:
            if (xevent->xbutton.button == Button1)
            {
                makeWidget(This);
            } 
            break;
        default:
            break;
    }
}
#endif

static void
addXtEventHandler(PluginInstance *This)
{
#ifdef MOZ_X11
     Display *dpy = (Display*) This->display;
     Window xwin = (Window) This->window;
     Widget xt_w = XtWindowToWidget(dpy, xwin);
     if (xt_w)
     {
         long event_mask = ExposureMask | ButtonReleaseMask | ButtonPressMask;
         XSelectInput(dpy, xwin, event_mask);
         XtAddEventHandler(xt_w, event_mask, False, (XtEventHandler)xt_event_handler, This);
     }
#endif
}


void
makePixmap(PluginInstance *This)
{
    createPixmap(This);
    drawPixmap(This);
    setCursor(This);
    addXtEventHandler(This);
}
