





































#include "nsWidget.h"
#include "keysym2ucs.h"
#include "nsWindow.h"
#include "nsAppShell.h"

#include "nsGUIEvent.h"

#include "nsGtkIMEHelper.h"


#include <stdio.h>

#include <gtk/gtk.h>
#include <gtk/gtkprivate.h>
#include "nsGtkEventHandler.h"

#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef DEBUG_pavlov

#endif



static void
dispatch_superwin_event(GdkEvent *event, nsWindow *window);

static PRBool
gdk_window_child_of_gdk_window(GdkWindow *window, GdkWindow *ancestor);





static PRBool suppressNextKeyDown = PR_FALSE;


void InitAllocationEvent(GtkAllocation *aAlloc,
                         nsSizeEvent &anEvent)
{
  if (aAlloc != nsnull) {
    
    
    nsRect *foo = new nsRect(0, 0, aAlloc->width, aAlloc->height);
    anEvent.windowSize = foo;
    
    
    
    anEvent.mWinWidth = aAlloc->width;
    anEvent.mWinHeight = aAlloc->height;
  }

  anEvent.time = PR_IntervalNow();
}


struct nsKeyConverter {
  int vkCode; 
  int keysym; 
};





struct nsKeyConverter nsKeycodes[] = {
  { NS_VK_CANCEL,     GDK_Cancel },
  { NS_VK_BACK,       GDK_BackSpace },
  { NS_VK_TAB,        GDK_Tab },
  { NS_VK_TAB,        GDK_ISO_Left_Tab },
  { NS_VK_CLEAR,      GDK_Clear },
  { NS_VK_RETURN,     GDK_Return },
  { NS_VK_SHIFT,      GDK_Shift_L },
  { NS_VK_SHIFT,      GDK_Shift_R },
  { NS_VK_CONTROL,    GDK_Control_L },
  { NS_VK_CONTROL,    GDK_Control_R },
  { NS_VK_ALT,        GDK_Alt_L },
  { NS_VK_ALT,        GDK_Alt_R },
  { NS_VK_META,       GDK_Meta_L },
  { NS_VK_META,       GDK_Meta_R },
  { NS_VK_PAUSE,      GDK_Pause },
  { NS_VK_CAPS_LOCK,  GDK_Caps_Lock },
  { NS_VK_ESCAPE,     GDK_Escape },
  { NS_VK_SPACE,      GDK_space },
  { NS_VK_PAGE_UP,    GDK_Page_Up },
  { NS_VK_PAGE_DOWN,  GDK_Page_Down },
  { NS_VK_END,        GDK_End },
  { NS_VK_HOME,       GDK_Home },
  { NS_VK_LEFT,       GDK_Left },
  { NS_VK_UP,         GDK_Up },
  { NS_VK_RIGHT,      GDK_Right },
  { NS_VK_DOWN,       GDK_Down },
  { NS_VK_PRINTSCREEN, GDK_Print },
  { NS_VK_INSERT,     GDK_Insert },
  { NS_VK_DELETE,     GDK_Delete },

  
  { NS_VK_LEFT,       GDK_KP_Left },
  { NS_VK_RIGHT,      GDK_KP_Right },
  { NS_VK_UP,         GDK_KP_Up },
  { NS_VK_DOWN,       GDK_KP_Down },
  { NS_VK_PAGE_UP,    GDK_KP_Page_Up },
    
    
    
    
    
  { NS_VK_PAGE_DOWN,  GDK_KP_Page_Down },
  { NS_VK_HOME,       GDK_KP_Home },
  { NS_VK_END,        GDK_KP_End },
  { NS_VK_INSERT,     GDK_KP_Insert },
  { NS_VK_DELETE,     GDK_KP_Delete },

  { NS_VK_MULTIPLY,   GDK_KP_Multiply },
  { NS_VK_ADD,        GDK_KP_Add },
  { NS_VK_SEPARATOR,  GDK_KP_Separator },
  { NS_VK_SUBTRACT,   GDK_KP_Subtract },
  { NS_VK_DECIMAL,    GDK_KP_Decimal },
  { NS_VK_DIVIDE,     GDK_KP_Divide },
  { NS_VK_RETURN,     GDK_KP_Enter },
  { NS_VK_NUM_LOCK,   GDK_Num_Lock },
  { NS_VK_SCROLL_LOCK,GDK_Scroll_Lock },

  { NS_VK_COMMA,      GDK_comma },
  { NS_VK_PERIOD,     GDK_period },
  { NS_VK_SLASH,      GDK_slash },
  { NS_VK_BACK_SLASH, GDK_backslash },
  { NS_VK_BACK_QUOTE, GDK_grave },
  { NS_VK_OPEN_BRACKET, GDK_bracketleft },
  { NS_VK_CLOSE_BRACKET, GDK_bracketright },
  { NS_VK_SEMICOLON, GDK_colon },
  { NS_VK_QUOTE, GDK_apostrophe },

  
  
  { NS_VK_CONTEXT_MENU, GDK_Menu },

  
  
  { NS_VK_SUBTRACT, GDK_minus },
  { NS_VK_EQUALS, GDK_equal },

  
  
  { NS_VK_QUOTE, GDK_quotedbl },
  { NS_VK_OPEN_BRACKET, GDK_braceleft },
  { NS_VK_CLOSE_BRACKET, GDK_braceright },
  { NS_VK_BACK_SLASH, GDK_bar },
  { NS_VK_SEMICOLON, GDK_semicolon },
  { NS_VK_BACK_QUOTE, GDK_asciitilde },
  { NS_VK_COMMA, GDK_less },
  { NS_VK_PERIOD, GDK_greater },
  { NS_VK_SLASH,      GDK_question },
  { NS_VK_1, GDK_exclam },
  { NS_VK_2, GDK_at },
  { NS_VK_3, GDK_numbersign },
  { NS_VK_4, GDK_dollar },
  { NS_VK_5, GDK_percent },
  { NS_VK_6, GDK_asciicircum },
  { NS_VK_7, GDK_ampersand },
  { NS_VK_8, GDK_asterisk },
  { NS_VK_9, GDK_parenleft },
  { NS_VK_0, GDK_parenright },
  { NS_VK_SUBTRACT, GDK_underscore },
  { NS_VK_EQUALS, GDK_plus }
};

#define IS_XSUN_XSERVER(dpy) \
    (strstr(XServerVendor(dpy), "Sun Microsystems") != NULL)


struct nsKeyConverter nsSunKeycodes[] = {
  {NS_VK_ESCAPE, GDK_F11 }, 
  {NS_VK_F11, 0x1005ff10 }, 
  {NS_VK_F12, 0x1005ff11 }, 
  {NS_VK_PAGE_UP,    GDK_F29 }, 
  {NS_VK_PAGE_DOWN,  GDK_F35 }, 
  {NS_VK_HOME,       GDK_F27 }, 
  {NS_VK_END,        GDK_F33 }, 
};




int nsPlatformToDOMKeyCode(GdkEventKey *aGEK)
{
  int i, length = 0;
  int keysym = aGEK->keyval;

  
  
  

  
  
  if (keysym >= GDK_a && keysym <= GDK_z)
    return keysym - GDK_a + NS_VK_A;
  if (keysym >= GDK_A && keysym <= GDK_Z)
    return keysym - GDK_A + NS_VK_A;

  
  if (keysym >= GDK_0 && keysym <= GDK_9)
    return keysym - GDK_0 + NS_VK_0;

  
  if (keysym >= GDK_KP_0 && keysym <= GDK_KP_9)
    return keysym - GDK_KP_0 + NS_VK_NUMPAD0;

  
  if (IS_XSUN_XSERVER(GDK_DISPLAY())) {
    length = sizeof(nsSunKeycodes) / sizeof(struct nsKeyConverter);
    for (i = 0; i < length; i++) {
      if (nsSunKeycodes[i].keysym == keysym)
        return(nsSunKeycodes[i].vkCode);
    }
  }

  
  length = sizeof(nsKeycodes) / sizeof(struct nsKeyConverter);
  for (i = 0; i < length; i++) {
    if (nsKeycodes[i].keysym == keysym)
      return(nsKeycodes[i].vkCode);
  }

  if (keysym >= GDK_F1 && keysym <= GDK_F24)
    return keysym - GDK_F1 + NS_VK_F1;

#if defined(DEBUG_akkana) || defined(DEBUG_ftang)
  printf("No match in nsPlatformToDOMKeyCode: keysym is 0x%x, string is '%s', keyval = %d\n", keysym, aGEK->string, aGEK->keyval);
#endif

  return((int)0);
}




PRUint32 nsConvertCharCodeToUnicode(GdkEventKey* aGEK)
{
  
  
  if (aGEK->keyval > 0xf000 && (aGEK->keyval & 0xff000000) != 0x01000000) {
    
    
    switch (aGEK->keyval)
    {
      case GDK_KP_Space:
        return ' ';
      case GDK_KP_Equal:
        return '=';
      case GDK_KP_Multiply:
        return '*';
      case GDK_KP_Add:
        return '+';
      case GDK_KP_Separator:
        return ',';
      case GDK_KP_Subtract:
        return '-';
      case GDK_KP_Decimal:
        return '.';
      case GDK_KP_Divide:
        return '/';
      case GDK_KP_0:
        return '0';
      case GDK_KP_1:
        return '1';
      case GDK_KP_2:
        return '2';
      case GDK_KP_3:
        return '3';
      case GDK_KP_4:
        return '4';
      case GDK_KP_5:
        return '5';
      case GDK_KP_6:
        return '6';
      case GDK_KP_7:
        return '7';
      case GDK_KP_8:
        return '8';
      case GDK_KP_9:
        return '9';
    }

    
    return 0;
  }

#if defined(USE_XIM) && defined(_AIX)
  
  
  
  
  

  PRBool controlChar = (aGEK->state & GDK_CONTROL_MASK ||
                        aGEK->state & GDK_MOD1_MASK ||
                        aGEK->state & GDK_MOD4_MASK);

  
  
  
  
  

  if (!controlChar && gdk_im_ready() && aGEK->length > 0 
        && aGEK->keyval != (guint)*aGEK->string) {
    nsGtkIMEHelper* IMEHelper = nsGtkIMEHelper::GetSingleton();
    if (IMEHelper != nsnull) {
      PRUnichar* unichars = IMEHelper->GetUnichars();
      PRInt32 unilen = IMEHelper->GetUnicharsSize();
      PRInt32 unichar_size = IMEHelper->MultiByteToUnicode(
                                          aGEK->string, aGEK->length,
                                          &unichars, &unilen);
      if (unichar_size > 0) {
        IMEHelper->SetUnichars(unichars);
        IMEHelper->SetUnicharsSize(unilen);
        return (long)*unichars;
      }
    }
  }
#endif
  
  long ucs = keysym2ucs(aGEK->keyval);
  if ((ucs != -1) && (ucs < 0x10000))
    return ucs;

  
  return 0;
}


void InitKeyEvent(GdkEventKey *aGEK,
                            nsKeyEvent &anEvent)
{
  if (aGEK != nsnull) {
    anEvent.keyCode = nsPlatformToDOMKeyCode(aGEK);
    anEvent.time = aGEK->time;
    anEvent.isShift = (aGEK->state & GDK_SHIFT_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.isControl = (aGEK->state & GDK_CONTROL_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.isAlt = (aGEK->state & GDK_MOD1_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.isMeta = (aGEK->state & GDK_MOD4_MASK) ? PR_TRUE : PR_FALSE;
  }
}

void InitKeyPressEvent(GdkEventKey *aGEK,
                       nsKeyEvent &anEvent)
{
  
  
  

  if (aGEK!=nsnull) {
    anEvent.isShift = (aGEK->state & GDK_SHIFT_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.isControl = (aGEK->state & GDK_CONTROL_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.isAlt = (aGEK->state & GDK_MOD1_MASK) ? PR_TRUE : PR_FALSE;
    anEvent.isMeta = (aGEK->state & GDK_MOD4_MASK) ? PR_TRUE : PR_FALSE;

    anEvent.charCode = nsConvertCharCodeToUnicode(aGEK);
    if (anEvent.charCode) {
      
      
      
      

      if ( anEvent.isControl || anEvent.isAlt || anEvent.isMeta ) {
         
         
         
         
         
         
         
        if ( (anEvent.charCode >= GDK_A && anEvent.charCode <= GDK_Z) ||
             (anEvent.charCode >= GDK_a && anEvent.charCode <= GDK_z) ) {
          anEvent.charCode = (anEvent.isShift) ? 
            gdk_keyval_to_upper(anEvent.charCode) : 
            gdk_keyval_to_lower(anEvent.charCode);
        }
      }
    } else {
      anEvent.keyCode = nsPlatformToDOMKeyCode(aGEK);
    }

#if defined(DEBUG_akkana_not) || defined (DEBUG_ftang)
    if (!aGEK->length) printf("!length, ");
    printf("Key Press event: gtk string = '%s', keyval = '%c' = %d,\n",
           aGEK->string, aGEK->keyval, aGEK->keyval);
    printf("    --> keyCode = 0x%x, char code = '%c'",
           anEvent.keyCode, anEvent.charCode);
    if (anEvent.isShift)
      printf(" [shift]");
    if (anEvent.isControl)
      printf(" [ctrl]");
    if (anEvent.isAlt)
      printf(" [alt]");
    if (anEvent.isMeta)
      printf(" [meta]");
    printf("\n");
#endif

    anEvent.time = aGEK->time;
  }
}






void handle_size_allocate(GtkWidget *w, GtkAllocation *alloc, gpointer p)
{
  nsWindow *widget = (nsWindow *)p;
  nsSizeEvent event(PR_TRUE, NS_SIZE, widget);

  InitAllocationEvent(alloc, event);
  NS_ADDREF(widget);
  widget->OnResize(&event);
  NS_RELEASE(widget);

  delete event.windowSize;
}


gint handle_key_press_event(GtkObject *w, GdkEventKey* event, gpointer p)
{
  nsWidget *win = (nsWidget*)p;

  
  if (nsWidget::sFocusWindow)
    win = nsWidget::sFocusWindow;

  
  if (event->keyval == GDK_Tab)
    if (event->state & GDK_CONTROL_MASK)
      if (event->state & GDK_MOD1_MASK)
        return PR_FALSE;


  NS_ADDREF(win);

  
  
  
  
  
  PRBool noDefault = PR_FALSE;
  nsKeyEvent keyDownEvent(PR_TRUE, NS_KEY_DOWN, win);
  InitKeyEvent(event, keyDownEvent);
  
  if (suppressNextKeyDown == PR_TRUE)
    suppressNextKeyDown = PR_FALSE;
  else
    noDefault = win->OnKey(keyDownEvent);

  
  if (event->keyval == GDK_Shift_L
      || event->keyval == GDK_Shift_R
      || event->keyval == GDK_Control_L
      || event->keyval == GDK_Control_R
      || event->keyval == GDK_Alt_L
      || event->keyval == GDK_Alt_R
      || event->keyval == GDK_Meta_L
      || event->keyval == GDK_Meta_R)
    return PR_TRUE;

  
  
  
  
  

  
  nsKeyEvent keyPressEvent(PR_TRUE, NS_KEY_PRESS, win);
  InitKeyPressEvent(event, keyPressEvent);
  if (noDefault) {  
    keyPressEvent.flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }

  if (event->length) {
    if (keyPressEvent.charCode || keyPressEvent.keyCode) {
      
      
      win->OnKey(keyPressEvent);
    } else if (nsGtkIMEHelper::GetSingleton()) {
      
      win->IMECommitEvent(event);
    }
  } else { 
    win->OnKey(keyPressEvent);
  }

  NS_RELEASE(win);

  if (w)
  {
    gtk_signal_emit_stop_by_name (GTK_OBJECT(w), "key_press_event");
  }

  return PR_TRUE;
}


gint handle_key_release_event(GtkObject *w, GdkEventKey* event, gpointer p)
{
  XEvent nextEvent;
  PRBool    shouldDrop = PR_FALSE;
  
  
  
  
  
  
  

  
  

  if (XPending(GDK_DISPLAY())) {
    
    XPeekEvent(GDK_DISPLAY(), &nextEvent);
    
    
    
    if ((nextEvent.xany.type == KeyPress) &&
        (nextEvent.xkey.time == event->time))
    {
      shouldDrop = PR_TRUE;
      
      
      suppressNextKeyDown = PR_TRUE;
    }
  }

  
  if (shouldDrop)
    return PR_TRUE;

  nsWidget *win = (nsWidget *)p;
  if (nsWidget::sFocusWindow)
    win = nsWidget::sFocusWindow;

  nsKeyEvent kevent(PR_TRUE, NS_KEY_UP, win);
  InitKeyEvent(event, kevent);

  NS_ADDREF(win);
  win->OnKey(kevent);
  NS_RELEASE(win);

  if (w)
  {
    gtk_signal_emit_stop_by_name (GTK_OBJECT(w), "key_release_event");
  }

  return PR_TRUE;
}


void 
handle_gdk_event (GdkEvent *event, gpointer data)
{
  GtkObject *eventObject = nsnull;

  
  
  guint32 event_time = gdk_event_get_time(event);
  if (event_time)
    nsWidget::SetLastEventTime(event_time);

  
  
  
  
  
  unsigned long serial = 0;

  if (XPending(GDK_DISPLAY())) {
    XEvent temp_event;
    XPeekEvent(GDK_DISPLAY(), &temp_event);
    serial = temp_event.xany.serial - 1;
  }

  
  if (event->any.window)
    gdk_window_get_user_data (event->any.window, (void **)&eventObject);

  
  
  
  

  if (eventObject && GDK_IS_SUPERWIN(eventObject)) {
    
    nsWindow *window = (nsWindow *)gtk_object_get_data (eventObject,
                                                        "nsWindow");

    
    
    if (!window)
      goto end;

    
    
    
    
    PRBool rewriteEvent = PR_FALSE;
    GtkWidget *grabWidget = gtk_grab_get_current();
    GtkWidget *owningWidget = window->GetOwningWidget();

    if (grabWidget &&
        !GTK_IS_MOZAREA(grabWidget) &&
        !gdk_window_child_of_gdk_window(owningWidget->window,
                                        grabWidget->window)) {
      rewriteEvent = PR_TRUE;
    }


    
    
    switch(event->type)
      {
      case GDK_NOTHING:
        break;

      case GDK_DESTROY:
      case GDK_DELETE:
      case GDK_PROPERTY_NOTIFY:
      case GDK_EXPOSE:
      case GDK_NO_EXPOSE:
      case GDK_FOCUS_CHANGE:
      case GDK_CONFIGURE:
      case GDK_MAP:
      case GDK_UNMAP:
      case GDK_SELECTION_CLEAR:
      case GDK_SELECTION_REQUEST:
      case GDK_SELECTION_NOTIFY:
      case GDK_CLIENT_EVENT:
      case GDK_VISIBILITY_NOTIFY:
        dispatch_superwin_event(event, window);
        break;

      case GDK_BUTTON_PRESS:
      case GDK_2BUTTON_PRESS:
      case GDK_3BUTTON_PRESS:
      case GDK_KEY_PRESS:
      case GDK_KEY_RELEASE:
        
        
        if (rewriteEvent) {
          gdk_window_unref(event->any.window);
          event->any.window = owningWidget->window;
          gdk_window_ref(event->any.window);
          gtk_main_do_event(event);
          break;
        }

        
        if (GTK_WIDGET_IS_SENSITIVE(owningWidget))
          dispatch_superwin_event(event, window);
        break;

      case GDK_MOTION_NOTIFY:
      case GDK_BUTTON_RELEASE:
      case GDK_PROXIMITY_IN:
      case GDK_PROXIMITY_OUT:
        
        if (rewriteEvent) {
          gdk_window_unref(event->any.window);
          event->any.window = owningWidget->window;
          gdk_window_ref(event->any.window);
          gtk_propagate_event(grabWidget, event);
          break;
        }

        if (GTK_WIDGET_IS_SENSITIVE(owningWidget))
          dispatch_superwin_event(event, window);
        break;

      case GDK_ENTER_NOTIFY:
      case GDK_LEAVE_NOTIFY:
        
        
        
        
        dispatch_superwin_event(event, window);
        break;

      default:
        
        NS_WARNING("Odd, hit default case in handle_gdk_event()\n");
        break;
      }
  }
  else {
    nsWindow  *grabbingWindow = nsWindow::GetGrabWindow();
    
    nsCOMPtr<nsIWidget> grabbingWindowGuard(grabbingWindow);
    GtkWidget *tempWidget = NULL;
    
    if (grabbingWindow) {
      
      GdkWindow *grabbingGdkWindow =
        NS_STATIC_CAST(GdkWindow *,
                       grabbingWindow->GetNativeData(NS_NATIVE_WINDOW));
      
      
      
      
      
      
      
      
      
      if (GTK_IS_WIDGET(eventObject)) {
        tempWidget = GTK_WIDGET(eventObject);
        if (gdk_window_child_of_gdk_window(tempWidget->window,
                                           grabbingGdkWindow)) {
          
          
          
          
          
          
          
          
          
          
          
          
          
          if (tempWidget->parent) {
            if (GTK_IS_MOZBOX(tempWidget->parent)) {
              tempWidget = tempWidget->parent;
            }
          }
          gtk_grab_add(tempWidget);
        }
        else  {
          
          
          dispatch_superwin_event(event, grabbingWindow);
          goto end;
        }
      }
    }

    gtk_main_do_event(event);

    if (tempWidget)
      gtk_grab_remove(tempWidget);

    if (event->type == GDK_BUTTON_RELEASE) {
      
      
      
      
      
      nsWidget::DropMotionTarget();
    }
  }

 end:
  ;

#if 0
  
  
  if (serial)
    nsAppShell::ProcessBeforeID(serial);
#endif

}

void
dispatch_superwin_event(GdkEvent *event, nsWindow *window)
{
  if (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE)
  {
    
    
    
    if (!nsWidget::sFocusWindow)
    {
      GtkWidget *mozArea = window->GetOwningWidget();
      NS_ASSERTION(mozArea, "Failed to get GtkMozArea for superwin event!\n");
      
      GtkWidget *toplevel = gtk_widget_get_toplevel(mozArea);
      
      gboolean handled = gtk_widget_event(toplevel, event);
      if (handled)
        return;
    }
  }

  switch (event->type)
  {
  case GDK_KEY_PRESS:
    handle_key_press_event (NULL, &event->key, window);
    break;
  case GDK_KEY_RELEASE:
    handle_key_release_event (NULL, &event->key, window);
    break;
  default:
    window->HandleGDKEvent (event);
    break;
  }
}

PRBool
gdk_window_child_of_gdk_window(GdkWindow *window, GdkWindow *ancestor)
{
  GdkWindow *this_window = window;
  while (this_window)
  {
    if (this_window == ancestor)
      return PR_TRUE;
    this_window = gdk_window_get_parent(this_window);
  }
  return PR_FALSE;
}



void
handle_xlib_shell_event(GdkSuperWin *superwin, XEvent *event, gpointer p)
{
  nsWindow *window = (nsWindow *)p;
  switch(event->xany.type) {
  case ConfigureNotify:
    window->HandleXlibConfigureNotifyEvent(event);
    break;
  default:
    break;
  }
}


void
handle_superwin_paint(gint aX, gint aY,
                      gint aWidth, gint aHeight, gpointer aData)
{
  nsWindow *window = (nsWindow *)aData;
  nsRect    rect;
  rect.x = aX;
  rect.y = aY;
  rect.width = aWidth;
  rect.height = aHeight;
  window->Invalidate(rect, PR_FALSE);
}

void
handle_superwin_flush(gpointer aData)
{
  nsWindow *window = (nsWindow *)aData;
  window->Update();
}



