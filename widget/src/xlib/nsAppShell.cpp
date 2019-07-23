









































#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xlocale.h>

#include "nsWindow.h"
#include "nsWidget.h"
#include "nsAppShell.h"
#include "nsKeyCode.h"
#include "nsWidgetsCID.h"

#include "nsIWidget.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsIDragService.h"
#include "nsIDragSessionXlib.h"
#include "nsITimer.h"

#include "xlibrgb.h"

#define CHAR_BUF_SIZE 80

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);


Display *nsAppShell::mDisplay = nsnull;
XlibRgbHandle *nsAppShell::mXlib_rgb_handle = nsnull;
XtAppContext nsAppShell::mAppContext;
PRTime nsAppShell::mClickTime = 0;
PRInt16 nsAppShell::mClicks = 1;
PRUint16 nsAppShell::mClickedButton = 0;
PRPackedBool nsAppShell::mClicked = PR_FALSE;
PRPackedBool nsAppShell::mDragging  = PR_FALSE;
PRPackedBool nsAppShell::mAltDown   = PR_FALSE;
PRPackedBool nsAppShell::mShiftDown = PR_FALSE;
PRPackedBool nsAppShell::mCtrlDown  = PR_FALSE;
PRPackedBool nsAppShell::mMetaDown  = PR_FALSE;
PRPackedBool nsAppShell::DieAppShellDie = PR_FALSE;

static PLHashTable *sQueueHashTable = nsnull;
static PLHashTable *sCountHashTable = nsnull;
static nsVoidArray *sEventQueueList = nsnull;



static const char *event_names[] = 
{
  "",
  "",
  "KeyPress",
  "KeyRelease",
  "ButtonPress",
  "ButtonRelease",
  "MotionNotify",
  "EnterNotify",
  "LeaveNotify",
  "FocusIn",
  "FocusOut",
  "KeymapNotify",
  "Expose",
  "GraphicsExpose",
  "NoExpose",
  "VisibilityNotify",
  "CreateNotify",
  "DestroyNotify",
  "UnmapNotify",
  "MapNotify",
  "MapRequest",
  "ReparentNotify",
  "ConfigureNotify",
  "ConfigureRequest",
  "GravityNotify",
  "ResizeRequest",
  "CirculateNotify",
  "CirculateRequest",
  "PropertyNotify",
  "SelectionClear",
  "SelectionRequest",
  "SelectionNotify",
  "ColormapNotify",
  "ClientMessage",
  "MappingNotify"
};

#define COMPARE_FLAG1( a,b) ((b)[0]=='-' && !strcmp((a), &(b)[1]))
#define COMPARE_FLAG2( a,b) ((b)[0]=='-' && (b)[1]=='-' && !strcmp((a), &(b)[2]))
#define COMPARE_FLAG12(a,b) ((b)[0]=='-' && !strcmp((a), (b)[1]=='-'?&(b)[2]:&(b)[1]))

#define ALL_EVENTS ( KeyPressMask | KeyReleaseMask | ButtonPressMask | \
                     ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | \
                     PointerMotionMask | PointerMotionHintMask | Button1MotionMask | \
                     Button2MotionMask | Button3MotionMask | \
                     Button4MotionMask | Button5MotionMask | ButtonMotionMask | \
                     KeymapStateMask | ExposureMask | VisibilityChangeMask | \
                     StructureNotifyMask | ResizeRedirectMask | \
                     SubstructureNotifyMask | SubstructureRedirectMask | \
                     FocusChangeMask | PropertyChangeMask | \
                     ColormapChangeMask | OwnerGrabButtonMask )

nsAppShell::nsAppShell()  
{ 
  if (!sEventQueueList)
    sEventQueueList = new nsVoidArray();

  mEventQueue = nsnull;
}

NS_IMPL_ISUPPORTS1(nsAppShell, nsIAppShell)

PR_BEGIN_EXTERN_C
static 
int xerror_handler( Display *display, XErrorEvent *ev )
{
  
  char errmsg[80];
  XGetErrorText(display, ev->error_code, errmsg, sizeof(errmsg));
  fprintf(stderr, "nsAppShellXlib: Warning (X Error) -  %s\n", errmsg);
  abort(); 
  
  return 0;
}
PR_END_EXTERN_C

NS_METHOD nsAppShell::Create(int* bac, char ** bav)
{
  
  if (mAppContext == nsnull) {
    int      argc = bac ? *bac : 0;
    char   **argv = bav;
    nsresult rv;

    char        *displayName    = nsnull;
    Bool         synchronize    = False;
    int          i;
    XlibRgbArgs  xargs;
    memset(&xargs, 0, sizeof(xargs));
    

    xargs.handle_name = XXLIBRGB_DEFAULT_HANDLE;

    for (i = 0; ++i < argc-1; ) {
      
      if (COMPARE_FLAG12 ("display", argv[i])) {
        displayName=argv[i+1];
        break;
      }
    }
    for (i = 0; ++i < argc-1; ) {
      if (COMPARE_FLAG1 ("visual", argv[i])) {
        xargs.xtemplate_mask |= VisualIDMask;
        xargs.xtemplate.visualid = strtol(argv[i+1], NULL, 0);
        break;
      }
    }   
    for (i = 0; ++i < argc; ) {
      if (COMPARE_FLAG1 ("sync", argv[i])) {
        synchronize = True;
        break;
      }
    }
    for (i = 0; ++i < argc; ) {
      
      if (COMPARE_FLAG12 ("no-xshm", argv[i])) {
        xargs.disallow_mit_shmem = True;
        break;
      }
    }    
    for (i = 0; ++i < argc; ) {
      if (COMPARE_FLAG1 ("install_colormap", argv[i])) {
        xargs.install_colormap = True;
        break;
      }
    }
    
    
    if (!setlocale (LC_ALL,""))
      NS_WARNING("locale not supported by C library");
  
    if (!XSupportsLocale ()) {
      NS_WARNING("locale not supported by Xlib, locale set to C");
      setlocale (LC_ALL, "C");
    }
  
    if (!XSetLocaleModifiers (""))
      NS_WARNING("can not set locale modifiers");

    XtToolkitInitialize();
    mAppContext = XtCreateApplicationContext();

    if (!(mDisplay = XtOpenDisplay (mAppContext, displayName, 
                                    "Mozilla5", "Mozilla5", nsnull, 0, 
                                    &argc, argv))) 
    {
      fprintf (stderr, "%s:  unable to open display \"%s\"\n", argv[0], XDisplayName(displayName));
      exit (EXIT_FAILURE);
    }
    
    
    
    if (synchronize)
    {
      

      (void)XSetErrorHandler(xerror_handler);
      
      NS_WARNING("running via unbuffered X connection.");
      XSynchronize(mDisplay, True);
    }
       
    mXlib_rgb_handle = xxlib_rgb_create_handle(mDisplay, XDefaultScreenOfDisplay(mDisplay),
                                               &xargs);
    if (!mXlib_rgb_handle)
    {
      fprintf (stderr, "%s:  unable to create Xlib context\n", argv[0]);
      exit (EXIT_FAILURE);
    }
  }

  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsAppShell::Create(dpy=%p)\n",
         mDisplay));

  return NS_OK;
}

NS_IMETHODIMP nsAppShell::Spinup()
{
  nsresult rv = NS_OK;

#ifdef DEBUG_APPSHELL
  printf("nsAppShell::Spinup()\n");
#endif

  
  nsCOMPtr<nsIEventQueueService> eventQService = do_GetService(kEventQueueServiceCID, &rv);

  if (NS_FAILED(rv)) {
    NS_WARNING("Could not obtain event queue service");
    return rv;
  }

  
  rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(mEventQueue));
  
  
  if (!mEventQueue) {
    
    rv = eventQService->CreateThreadEventQueue();
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not create the thread event queue");
      return rv;
    }

    
    rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(mEventQueue));
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not get the thread event queue");
      return rv;
    }
  }

  ListenToEventQueue(mEventQueue, PR_TRUE);

  return rv;
}


PR_BEGIN_EXTERN_C
static
void HandleQueueXtProc(XtPointer ptr, int *source_fd, XtInputId* id)
{
  nsIEventQueue *queue = (nsIEventQueue *)ptr;
  queue->ProcessPendingEvents();
}
PR_END_EXTERN_C

nsresult nsAppShell::Run()
{
  if (mEventQueue == nsnull)
    Spinup();

  if (mEventQueue == nsnull) {
    NS_WARNING("Cannot initialize the Event Queue");
    return NS_ERROR_NOT_INITIALIZED;
  }
               
  XEvent xevent;
  
  
  while (!DieAppShellDie) 
  {   
    XtAppNextEvent(mAppContext, &xevent);
  
    if (XtDispatchEvent(&xevent) == False)
      DispatchXEvent(&xevent);
    
    if (XEventsQueued(mDisplay, QueuedAlready) == 0)
    {
      
      nsWindow::UpdateIdle(nsnull);
    }
  }
  
  Spindown();
  return NS_OK;
}

NS_METHOD nsAppShell::Spindown()
{
  if (mEventQueue) {
    ListenToEventQueue(mEventQueue, PR_FALSE);
    mEventQueue->ProcessPendingEvents();
    mEventQueue = nsnull;
  }

  return NS_OK;
}

#define NUMBER_HASH_KEY(_num) ((PLHashNumber) _num)

static PLHashNumber
IntHashKey(PRInt32 key)
{
  return NUMBER_HASH_KEY(key);
}

PR_BEGIN_EXTERN_C
static unsigned long getNextRequest (void *aClosure) {
  return XNextRequest(nsAppShell::mDisplay);
}
PR_END_EXTERN_C

NS_IMETHODIMP nsAppShell::ListenToEventQueue(nsIEventQueue *aQueue,
                                             PRBool aListen)
{
  if (!mEventQueue) {
    NS_WARNING("nsAppShell::ListenToEventQueue(): No event queue available.");
    return NS_ERROR_NOT_INITIALIZED;
  }
  
#ifdef DEBUG_APPSHELL
  printf("ListenToEventQueue(%p, %d) this=%p\n", aQueue, aListen, this);
#endif
  if (!sQueueHashTable) {
    sQueueHashTable = PL_NewHashTable(3, (PLHashFunction)IntHashKey,
                                      PL_CompareValues, PL_CompareValues, 0, 0);
  }
  if (!sCountHashTable) {
    sCountHashTable = PL_NewHashTable(3, (PLHashFunction)IntHashKey,
                                      PL_CompareValues, PL_CompareValues, 0, 0);
  }    

  int   queue_fd = aQueue->GetEventQueueSelectFD();
  void *key      = aQueue;
  if (aListen) {
    

    if (!PL_HashTableLookup(sQueueHashTable, key)) {
      long tag;
        
      
      tag = (long)XtAppAddInput(mAppContext,
                                queue_fd,
                                (XtPointer)(long)(XtInputReadMask),
                                HandleQueueXtProc,
                                (XtPointer)mEventQueue);




#define NEVER_BE_ZERO_MAGIC (54321) 
      tag += NEVER_BE_ZERO_MAGIC; 
      NS_ASSERTION(tag!=0, "tag is 0 while adding");
      
      if (tag) {
        PL_HashTableAdd(sQueueHashTable, key, (void *)tag);
      }
      
      PLEventQueue *plqueue;
      aQueue->GetPLEventQueue(&plqueue);
      PL_RegisterEventIDFunc(plqueue, getNextRequest, 0);
      sEventQueueList->AppendElement(plqueue);
    }
  } else {
       
    PLEventQueue *plqueue;
    aQueue->GetPLEventQueue(&plqueue);
    PL_UnregisterEventIDFunc(plqueue);
    sEventQueueList->RemoveElement(plqueue);

    int tag = long(PL_HashTableLookup(sQueueHashTable, key));
    if (tag) {
      tag -= NEVER_BE_ZERO_MAGIC;
      XtRemoveInput((XtInputId)tag);
      PL_HashTableRemove(sQueueHashTable, key);
    }  
  }

  return NS_OK;
}




NS_METHOD
nsAppShell::GetNativeEvent(PRBool &aRealEvent, void *&aEvent)
{
  aRealEvent = PR_FALSE;
  aEvent     = nsnull;

  return NS_OK;
}

nsresult nsAppShell::DispatchNativeEvent(PRBool aRealEvent, void *aEvent)
{
  XEvent xevent;
  
  if (!mEventQueue)
    return NS_ERROR_NOT_INITIALIZED;

#if 1
  

  
  mEventQueue->ProcessPendingEvents();  
#endif

  XtAppNextEvent(mAppContext, &xevent);
    
  if (XtDispatchEvent(&xevent) == False)
    DispatchXEvent(&xevent);
   
  if (XEventsQueued(mDisplay, QueuedAlready) == 0)
  {
    
    nsWindow::UpdateIdle(nsnull);
  }
    
  return NS_OK;
}

NS_METHOD nsAppShell::Exit()
{
  DieAppShellDie = PR_TRUE;
  return NS_OK;
}

nsAppShell::~nsAppShell()
{
}

void* nsAppShell::GetNativeData(PRUint32 aDataType)
{
  return nsnull;
}

void
nsAppShell::DispatchXEvent(XEvent *event)
{
  nsWidget *widget;
  widget = nsWidget::GetWidgetForWindow(event->xany.window);

  
  
  if (widget == nsnull)
    return;

  
  switch (event->type) 
  {
  case Expose:
    HandleExposeEvent(event, widget);
    break;

  case ConfigureNotify:
    
    
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("DispatchEvent: ConfigureNotify event for window 0x%lx %d %d %d %d\n",
                                         event->xconfigure.window,
                                         event->xconfigure.x, 
                                         event->xconfigure.y,
                                         event->xconfigure.width, 
                                         event->xconfigure.height));

    HandleConfigureNotifyEvent(event, widget);

    break;

  case ButtonPress:
  case ButtonRelease:
    HandleFocusInEvent(event, widget);
    HandleButtonEvent(event, widget);
    break;

  case MotionNotify:
    HandleMotionNotifyEvent(event, widget);
    break;

  case KeyPress:
    HandleKeyPressEvent(event, widget);
    break;
  case KeyRelease:
    HandleKeyReleaseEvent(event, widget);
    break;

  case FocusIn:
    HandleFocusInEvent(event, widget);
    break;

  case FocusOut:
    HandleFocusOutEvent(event, widget);
    break;

  case EnterNotify:
    HandleEnterEvent(event, widget);
    break;

  case LeaveNotify:
    HandleLeaveEvent(event, widget);
    break;

  case NoExpose:
    
    break;
  case VisibilityNotify:
    HandleVisibilityNotifyEvent(event, widget);
    break;
  case ClientMessage:
    HandleClientMessageEvent(event, widget);
    break;
  case SelectionRequest:
    HandleSelectionRequestEvent(event, widget);
    break;
  default:
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Unhandled window event: Window 0x%lx Got a %s event\n",
                                         event->xany.window, event_names[event->type]));

    break;
  }
}

void
nsAppShell::HandleMotionNotifyEvent(XEvent *event, nsWidget *aWidget)
{
  if (mDragging) {
    HandleDragMotionEvent(event, aWidget);
  }

  nsMouseEvent mevent(PR_TRUE, NS_MOUSE_MOVE, aWidget, nsMouseEvent::eReal);
  XEvent aEvent;

  mevent.refPoint.x = event->xmotion.x;
  mevent.refPoint.y = event->xmotion.y;

  mevent.isShift = mShiftDown;
  mevent.isControl = mCtrlDown;
  mevent.isAlt = mAltDown;
  mevent.isMeta = mMetaDown;
  
  Display * dpy = (Display *)aWidget->GetNativeData(NS_NATIVE_DISPLAY);
  Window win = (Window)aWidget->GetNativeData(NS_NATIVE_WINDOW);
  
  while(XCheckWindowEvent(dpy,
                          win,
                          ButtonMotionMask,
                          &aEvent)) {
    mevent.refPoint.x = aEvent.xmotion.x;
    mevent.refPoint.y = aEvent.xmotion.y;
  }
  NS_ADDREF(aWidget);
  aWidget->DispatchMouseEvent(mevent);
  NS_RELEASE(aWidget);
}

void
nsAppShell::HandleButtonEvent(XEvent *event, nsWidget *aWidget)
{
  PRUint32 eventType = 0;
  PRInt16 button = nsMouseEvent::eLeftButton;
  PRBool currentlyDragging = mDragging;
  nsMouseScrollEvent scrollEvent(PR_TRUE, NS_MOUSE_SCROLL, aWidget);

  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Button event for window 0x%lx button %d type %s\n",
                                       event->xany.window,
                                       event->xbutton.button,
                                       (event->type == ButtonPress ? "ButtonPress" : "ButtonRelease")));
  switch(event->type) {
  case ButtonPress:
    eventType = NS_MOUSE_BUTTON_DOWN;
    switch(event->xbutton.button) {
    case 1:
      button = nsMouseEvent::eLeftButton;
      mDragging = PR_TRUE;
      break;
    case 2:
      button = nsMouseEvent::eMiddleButton;
      break;
    case 3:
      

      eventType = NS_CONTEXTMENU;
      button = nsMouseEvent::eRightButton;
      break;
    case 4:
    case 5:
      scrollEvent.delta = (event->xbutton.button == 4) ? -3 : 3;
      scrollEvent.scrollFlags = nsMouseScrollEvent::kIsVertical;

      scrollEvent.refPoint.x = event->xbutton.x;
      scrollEvent.refPoint.y = event->xbutton.y;

      scrollEvent.isShift = mShiftDown;
      scrollEvent.isControl = mCtrlDown;
      scrollEvent.isAlt = mAltDown;
      scrollEvent.isMeta = mMetaDown;
      scrollEvent.time = PR_Now();
      NS_IF_ADDREF(aWidget);
      aWidget->DispatchWindowEvent(scrollEvent);
      NS_IF_RELEASE(aWidget);
      return;
    }
    break;
  case ButtonRelease:
    eventType = NS_MOUSE_BUTTON_UP;
    switch(event->xbutton.button) {
    case 1:
      button = nsMouseEvent::eLeftButton;
      mDragging = PR_FALSE;
      break;
    case 2:
      button = nsMouseEvent::eMiddleButton;
      break;
    case 3:
      button = nsMouseEvent::eRightButton;
      break;
    case 4:
    case 5:
      return;
    }
    break;
  }

  nsMouseEvent mevent(PR_TRUE, eventType, aWidget, nsMouseEvent::eReal);
  mevent.button = button;
  mevent.isShift = mShiftDown;
  mevent.isControl = mCtrlDown;
  mevent.isAlt = mAltDown;
  mevent.isMeta = mMetaDown;
  mevent.refPoint.x = event->xbutton.x;
  mevent.refPoint.y = event->xbutton.y;
  mevent.time = PR_Now();
  
  
  
  if (PR_Now() - mClickTime > 1000000)
    mClicked = PR_FALSE;               

  if (event->type == ButtonPress) {
    if (!mClicked) {
      mClicked = PR_TRUE;
      mClickTime = PR_Now();
      mClicks = 1;
      mClickedButton = event->xbutton.button;
    } else {
      mClickTime = PR_Now() - mClickTime;
      if ((mClickTime < 500000) && (mClickedButton == event->xbutton.button))
        mClicks = 2;
      else
        mClicks = 1;
      mClicked = PR_FALSE;
    }
  }

  if (currentlyDragging && !mDragging)
    HandleDragDropEvent(event, aWidget);

  mevent.clickCount = mClicks;
  NS_IF_ADDREF(aWidget);
  aWidget->DispatchMouseEvent(mevent);
  NS_IF_RELEASE(aWidget);
}

void
nsAppShell::HandleExposeEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Expose event for window 0x%lx %d %d %d %d\n", event->xany.window,
                                       event->xexpose.x, event->xexpose.y, event->xexpose.width, event->xexpose.height));

  nsRect dirtyRect(event->xexpose.x, event->xexpose.y, 
                   event->xexpose.width, event->xexpose.height);

  

  if (event->xexpose.count!=0) {
     XEvent txe;
     do {
        XWindowEvent(event->xany.display, event->xany.window, ExposureMask, (XEvent *)&txe);
        dirtyRect.UnionRect(dirtyRect, nsRect(txe.xexpose.x, txe.xexpose.y, 
                                              txe.xexpose.width, txe.xexpose.height));
     } while (txe.xexpose.count>0);
  }

  aWidget->Invalidate(dirtyRect, PR_FALSE);
}

void
nsAppShell::HandleConfigureNotifyEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("ConfigureNotify event for window 0x%lx %d %d %d %d\n",
                                       event->xconfigure.window,
                                       event->xconfigure.x, event->xconfigure.y,
                                       event->xconfigure.width, event->xconfigure.height));

  XEvent    config_event;
  while (XCheckTypedWindowEvent(event->xany.display, 
                                event->xany.window, 
                                ConfigureNotify,
                                &config_event) == True) {
    
    
    if (config_event.type == ConfigureNotify) 
      {
        *event = config_event;
        
        PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("DispatchEvent: Extra ConfigureNotify event for window 0x%lx %d %d %d %d\n",
                                             event->xconfigure.window,
                                             event->xconfigure.x, 
                                             event->xconfigure.y,
                                             event->xconfigure.width, 
                                             event->xconfigure.height));
      }
    else {
      PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("EVENT LOSSAGE\n"));
    }
  }

  nsSizeEvent sevent(PR_TRUE, NS_SIZE, aWidget);
  sevent.windowSize = new nsRect (event->xconfigure.x, event->xconfigure.y,
                                  event->xconfigure.width, event->xconfigure.height);
  sevent.refPoint.x = event->xconfigure.x;
  sevent.refPoint.y = event->xconfigure.y;
  sevent.mWinWidth = event->xconfigure.width;
  sevent.mWinHeight = event->xconfigure.height;
  
  NS_ADDREF(aWidget);
  aWidget->OnResize(sevent);
  NS_RELEASE(aWidget);
  delete sevent.windowSize;
}

PRUint32 nsConvertCharCodeToUnicode(XKeyEvent* xkey)
{
  

  
  
  

  KeySym         keysym;
  XComposeStatus compose;
  unsigned char  string_buf[CHAR_BUF_SIZE];
  int            len = 0;

  len = XLookupString(xkey, (char *)string_buf, CHAR_BUF_SIZE-1, &keysym, &compose);
  if (0 == len)
    return 0;

  if (xkey->state & ControlMask) {
    if (xkey->state & ShiftMask) {
      return (PRUint32)(string_buf[0] + 'A' - 1);
    }
    else {
      return (PRUint32)(string_buf[0] + 'a' - 1);
    }
  }
  if (!isprint(string_buf[0])) {
    return 0;
  }
  else {
    return (PRUint32)(string_buf[0]);
  }
}

void
nsAppShell::HandleKeyPressEvent(XEvent *event, nsWidget *aWidget)
{
  char      string_buf[CHAR_BUF_SIZE];
  int       len = 0;
  Window    focusWindow = None;
  nsWidget *focusWidget = 0;

  
  focusWindow = nsWidget::GetFocusWindow();
  if (focusWindow != None) {
    focusWidget = nsWidget::GetWidgetForWindow(focusWindow);
  }

  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("KeyPress event for window 0x%lx ( 0x%lx focus window )\n",
                                       event->xkey.window,
                                       focusWindow));

  
  if (focusWidget == 0) {
    return;
  }

  KeySym     keysym = nsKeyCode::ConvertKeyCodeToKeySym(event->xkey.display,
                                                        event->xkey.keycode);

  switch (keysym) {
    case XK_Alt_L:
    case XK_Alt_R:
      mAltDown = PR_TRUE;
      break;
    case XK_Control_L:
    case XK_Control_R:
      mCtrlDown = PR_TRUE;
      break;
    case XK_Shift_L:
    case XK_Shift_R:
      mShiftDown = PR_TRUE;
      break;
    case XK_Meta_L:
    case XK_Meta_R:
      mMetaDown = PR_TRUE;
      break;
    default:
      break;
  }

  
  if (nsKeyCode::KeyCodeIsModifier(event->xkey.keycode))
  {
    return;
  }

  nsKeyEvent keyEvent(PR_TRUE, NS_KEY_DOWN, focusWidget);

  XComposeStatus compose;

  len = XLookupString(&event->xkey, string_buf, CHAR_BUF_SIZE-1, &keysym, &compose);
  string_buf[len] = '\0';

  keyEvent.keyCode = nsKeyCode::ConvertKeySymToVirtualKey(keysym);
  keyEvent.time = event->xkey.time;
  keyEvent.isShift = (event->xkey.state & ShiftMask) ? PR_TRUE : PR_FALSE;
  keyEvent.isControl = (event->xkey.state & ControlMask) ? 1 : 0;
  keyEvent.isAlt = (event->xkey.state & Mod1Mask) ? 1 : 0;
  
  keyEvent.isMeta = (event->xkey.state & Mod1Mask) ? 1 : 0;

  
  
  
  

  PRBool noDefault = focusWidget->DispatchKeyEvent(keyEvent);

  nsKeyEvent pressEvent(PR_TRUE, NS_KEY_PRESS, focusWidget);
  pressEvent.keyCode = nsKeyCode::ConvertKeySymToVirtualKey(keysym);
  pressEvent.charCode = nsConvertCharCodeToUnicode(&event->xkey);
  pressEvent.time = event->xkey.time;
  pressEvent.isShift = (event->xkey.state & ShiftMask) ? PR_TRUE : PR_FALSE;
  pressEvent.isControl = (event->xkey.state & ControlMask) ? 1 : 0;
  pressEvent.isAlt = (event->xkey.state & Mod1Mask) ? 1 : 0;
  pressEvent.isMeta = (event->xkey.state & Mod1Mask) ? 1 : 0;
  if (noDefault) {   
    pressEvent.flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }

  focusWidget->DispatchKeyEvent(pressEvent);
}

void
nsAppShell::HandleKeyReleaseEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("KeyRelease event for window 0x%lx\n",
                                       event->xkey.window));

  KeySym     keysym = nsKeyCode::ConvertKeyCodeToKeySym(event->xkey.display,
                                                        event->xkey.keycode);

  switch (keysym) {
    case XK_Alt_L:
    case XK_Alt_R:
      mAltDown = PR_FALSE;
      break;
    case XK_Control_L:
    case XK_Control_R:
      mCtrlDown = PR_FALSE;
      break;
    case XK_Shift_L:
    case XK_Shift_R:
      mShiftDown = PR_FALSE;
      break;
    case XK_Meta_L:
    case XK_Meta_R:
      mMetaDown = PR_FALSE;
      break;
    default:
      break;
  }

  
  if (nsKeyCode::KeyCodeIsModifier(event->xkey.keycode))
  {
    return;
  }

  nsKeyEvent keyEvent(PR_TRUE, NS_KEY_UP, aWidget);

  keyEvent.keyCode = nsKeyCode::ConvertKeySymToVirtualKey(keysym);
  keyEvent.time = event->xkey.time;
  keyEvent.isShift = event->xkey.state & ShiftMask;
  keyEvent.isControl = (event->xkey.state & ControlMask) ? 1 : 0;
  keyEvent.isAlt = (event->xkey.state & Mod1Mask) ? 1 : 0;
  keyEvent.isMeta = (event->xkey.state & Mod1Mask) ? 1 : 0;
  keyEvent.refPoint.x = event->xkey.x;
  keyEvent.refPoint.y = event->xkey.y;

  NS_ADDREF(aWidget);

  aWidget->DispatchKeyEvent(keyEvent);

  NS_RELEASE(aWidget);
}

void
nsAppShell::HandleFocusInEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("FocusIn event for window 0x%lx\n",
                                       event->xfocus.window));
  nsFocusEvent focusEvent(PR_TRUE, NS_GOTFOCUS, aWidget);
  
  NS_ADDREF(aWidget);
  aWidget->DispatchWindowEvent(focusEvent);
  NS_RELEASE(aWidget);
}

void
nsAppShell::HandleFocusOutEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("FocusOut event for window 0x%lx\n",
                                       event->xfocus.window));
  nsFocusEvent focusEvent(PR_TRUE, NS_LOSTFOCUS, aWidget);

  NS_ADDREF(aWidget);
  aWidget->DispatchWindowEvent(focusEvent);
  NS_RELEASE(aWidget);
}


static inline int
is_wm_ungrab_enter(XCrossingEvent *event)
{
  return (NotifyGrab == event->mode) &&
    ((NotifyAncestor == event->detail) ||
     (NotifyVirtual == event->detail));
}

static inline int
is_wm_grab_leave(XCrossingEvent *event)
{
  return (NotifyGrab == event->mode) &&
    ((NotifyAncestor == event->detail) ||
     (NotifyVirtual == event->detail));
}

void
nsAppShell::HandleEnterEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Enter event for window 0x%lx\n",
                                       event->xcrossing.window));

  if (event->xcrossing.subwindow != None)
    return;

  if(is_wm_ungrab_enter(&event->xcrossing))
    return;

  if (mDragging) {
    HandleDragEnterEvent(event, aWidget);
  }

  nsMouseEvent enterEvent(PR_TRUE, NS_MOUSE_ENTER, aWidget,
                          nsMouseEvent::eReal);

  enterEvent.time = event->xcrossing.time;
  enterEvent.refPoint.x = nscoord(event->xcrossing.x);
  enterEvent.refPoint.y = nscoord(event->xcrossing.y);
  
  
  
  aWidget->SetFocus();

  NS_ADDREF(aWidget);
  aWidget->DispatchWindowEvent(enterEvent);
  NS_RELEASE(aWidget);
}

void
nsAppShell::HandleLeaveEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Leave event for window 0x%lx\n",
                                       event->xcrossing.window));

  if (event->xcrossing.subwindow != None)
    return;

  if(is_wm_grab_leave(&event->xcrossing))
    return;

  if (mDragging) {
    HandleDragLeaveEvent(event, aWidget);
  }

  nsMouseEvent leaveEvent(PR_TRUE, NS_MOUSE_EXIT, aWidget,
                          nsMouseEvent::eReal);

  leaveEvent.time = event->xcrossing.time;
  leaveEvent.refPoint.x = nscoord(event->xcrossing.x);
  leaveEvent.refPoint.y = nscoord(event->xcrossing.y);
  
  NS_ADDREF(aWidget);
  aWidget->DispatchWindowEvent(leaveEvent);
  NS_RELEASE(aWidget);
}

void nsAppShell::HandleVisibilityNotifyEvent(XEvent *event, nsWidget *aWidget)
{
#ifdef DEBUG
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("VisibilityNotify event for window 0x%lx ",
                                       event->xfocus.window));
  switch(event->xvisibility.state) {
  case VisibilityFullyObscured:
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Fully Obscured\n"));
    break;
  case VisibilityPartiallyObscured:
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Partially Obscured\n"));
    break;
  case VisibilityUnobscured:
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Unobscured\n"));
  }
#endif
  aWidget->SetVisibility(event->xvisibility.state);
}

void nsAppShell::HandleMapNotifyEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("MapNotify event for window 0x%lx\n",
                                       event->xmap.window));
  
  
}

void nsAppShell::HandleUnmapNotifyEvent(XEvent *event, nsWidget *aWidget)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("UnmapNotifyEvent for window 0x%lx\n",
                                       event->xunmap.window));
  
  
}

void nsAppShell::HandleClientMessageEvent(XEvent *event, nsWidget *aWidget)
{
  
#if defined(DEBUG_warren) || defined(DEBUG_quy)
  printf("handling client message\n");
#endif
  if (nsWidget::WMProtocolsInitialized) {
    if ((Atom)event->xclient.data.l[0] == nsWidget::WMDeleteWindow) {
#ifdef DEBUG
      printf("got a delete window event\n");
#endif       
      aWidget->OnDeleteWindow();
    }
  }
}

void nsAppShell::HandleSelectionRequestEvent(XEvent *event, nsWidget *aWidget)
{
  nsGUIEvent ev(PR_TRUE, 0, aWidget);

  ev.nativeMsg = (void *)event;

  aWidget->DispatchWindowEvent(ev);
}

void nsAppShell::HandleDragMotionEvent(XEvent *event, nsWidget *aWidget) {
  PRBool currentlyDragging = PR_FALSE;

  nsresult rv;
  nsCOMPtr<nsIDragService> dragService( do_GetService(kCDragServiceCID, &rv) );
  nsCOMPtr<nsIDragSessionXlib> dragServiceXlib;
  if (NS_SUCCEEDED(rv)) {
    dragServiceXlib = do_QueryInterface(dragService);
    if (dragServiceXlib) {
      dragServiceXlib->IsDragging(&currentlyDragging);
    }
  }

  if (currentlyDragging) {
    dragServiceXlib->UpdatePosition(event->xmotion.x, event->xmotion.y);

    dragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);

    nsMouseEvent mevent(PR_TRUE, NS_DRAGDROP_OVER, aWidget,
                        nsMouseEvent::eReal);
    mevent.refPoint.x = event->xmotion.x;
    mevent.refPoint.y = event->xmotion.y;

    NS_ADDREF(aWidget);
    aWidget->DispatchMouseEvent(mevent);
    NS_RELEASE(aWidget);
  }
}

void nsAppShell::HandleDragEnterEvent(XEvent *event, nsWidget *aWidget) {
  PRBool currentlyDragging = PR_FALSE;

  nsresult rv;
  nsCOMPtr<nsIDragService> dragService( do_GetService(kCDragServiceCID, &rv) );
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDragSessionXlib> dragServiceXlib;
    dragServiceXlib = do_QueryInterface(dragService);
    if (dragServiceXlib) {
      dragServiceXlib->IsDragging(&currentlyDragging);
    }
  }

  if (currentlyDragging) {
    nsMouseEvent enterEvent(PR_TRUE, NS_DRAGDROP_ENTER, aWidget,
                            nsMouseEvent::eReal);
  
    enterEvent.refPoint.x = event->xcrossing.x;
    enterEvent.refPoint.y = event->xcrossing.y;
  
    NS_ADDREF(aWidget);
    aWidget->DispatchWindowEvent(enterEvent);
    NS_RELEASE(aWidget);
  }
}

void nsAppShell::HandleDragLeaveEvent(XEvent *event, nsWidget *aWidget) {
  PRBool currentlyDragging = PR_FALSE;
  
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService( do_GetService(kCDragServiceCID, &rv) );

  
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDragSessionXlib> dragServiceXlib;
    dragServiceXlib = do_QueryInterface(dragService);
    if (dragServiceXlib) {
      dragServiceXlib->IsDragging(&currentlyDragging);
    }
  }

  if (currentlyDragging) {
    nsMouseEvent leaveEvent(PR_TRUE, NS_DRAGDROP_EXIT, aWidget,
                            nsMouseEvent::eReal);
  
    leaveEvent.refPoint.x = event->xcrossing.x;
    leaveEvent.refPoint.y = event->xcrossing.y;
  
    NS_ADDREF(aWidget);
    aWidget->DispatchWindowEvent(leaveEvent);
    NS_RELEASE(aWidget);
  }
}

void nsAppShell::HandleDragDropEvent(XEvent *event, nsWidget *aWidget) {
  PRBool currentlyDragging = PR_FALSE;

  nsresult rv;
  nsCOMPtr<nsIDragService> dragService( do_GetService(kCDragServiceCID, &rv) );

  
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDragSessionXlib> dragServiceXlib;
    dragServiceXlib = do_QueryInterface(dragService);
    if (dragServiceXlib) {
      dragServiceXlib->IsDragging(&currentlyDragging);
    }
  }

  if (currentlyDragging) {
    nsMouseEvent mevent(PR_TRUE, NS_DRAGDROP_DROP, aWidget,
                        nsMouseEvent::eReal);
    mevent.refPoint.x = event->xbutton.x;
    mevent.refPoint.y = event->xbutton.y;
  
    NS_IF_ADDREF(aWidget);
    aWidget->DispatchMouseEvent(mevent);
    NS_IF_RELEASE(aWidget);

    dragService->EndDragSession();
  }
}

void nsAppShell::ForwardEvent(XEvent *event, nsWidget *aWidget)
{
  nsGUIEvent ev(PR_TRUE, 0, aWidget);
  ev.nativeMsg = (void *)event;

  aWidget->DispatchWindowEvent(ev);
}

