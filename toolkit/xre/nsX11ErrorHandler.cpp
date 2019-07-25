





































#include "nsX11ErrorHandler.h"

#ifdef MOZ_IPC
#include "mozilla/plugins/PluginProcessChild.h"
using mozilla::plugins::PluginProcessChild;
#endif

#include "prenv.h"
#include "nsXULAppAPI.h"
#include "nsExceptionHandler.h"
#include "nsDebug.h"

#include "mozilla/X11Util.h"
#include <X11/Xlib.h>

#define BUFSIZE 2048 // What Xlib uses with XGetErrorDatabaseText

extern "C" {
static int
X11Error(Display *display, XErrorEvent *event) {
  nsCAutoString notes;
  char buffer[BUFSIZE];

  
  
  unsigned long age = NextRequest(display) - event->serial;

  
  nsCAutoString message;
  if (event->request_code < 128) {
    
    message.AppendInt(event->request_code);
  } else {
    

    
    
    
    
    
    
    Display *tmpDisplay = XOpenDisplay(NULL);
    if (tmpDisplay) {
      int nExts;
      char** extNames = XListExtensions(tmpDisplay, &nExts);
      if (extNames) {
        for (int i = 0; i < nExts; ++i) {
          int major_opcode, first_event, first_error;
          if (XQueryExtension(tmpDisplay, extNames[i],
                              &major_opcode, &first_event, &first_error)
              && major_opcode == event->request_code) {
            message.Append(extNames[i]);
            message.Append('.');
            message.AppendInt(event->minor_code);
            break;
          }
        }

        XFreeExtensionList(extNames);
      }
      XCloseDisplay(tmpDisplay);
    }
  }

  if (message.IsEmpty()) {
    buffer[0] = '\0';
  } else {
    XGetErrorDatabaseText(display, "XRequest", message.get(), "",
                          buffer, sizeof(buffer));
  }

  if (buffer[0]) {
    notes.Append(buffer);
  } else {
    notes.Append("Request ");
    notes.AppendInt(event->request_code);
    notes.Append('.');
    notes.AppendInt(event->minor_code);
  }

  notes.Append(": ");

  
  XGetErrorText(display, event->error_code, buffer, sizeof(buffer));
  notes.Append(buffer);

  
  
  
  
  
  
  
  
  
  if (age > 1) {
    
    
    
    if (XSynchronize(display, True) == XSynchronize(display, False)) {
      notes.Append("; sync");
    } else {
      notes.Append("; ");
      notes.AppendInt(PRUint32(age));
      notes.Append(" requests ago");
    }
  }

#ifdef MOZ_CRASHREPORTER
  switch (XRE_GetProcessType()) {
  case GeckoProcessType_Default:
    CrashReporter::AppendAppNotesToCrashReport(notes);
    break;
#ifdef MOZ_IPC
  case GeckoProcessType_Plugin:
    if (CrashReporter::GetEnabled()) {
      
      
      
      PluginProcessChild::AppendNotesToCrashReport(notes);
    }
    break;
#endif
  default: 
    ; 
  }
#endif

#ifdef DEBUG
  
  
  notes.Append("; id=0x");
  notes.AppendInt(PRUint32(event->resourceid), 16);
#ifdef MOZ_X11
  
  
  
  if (!PR_GetEnv("MOZ_X_SYNC")) {
    notes.Append("\nRe-running with MOZ_X_SYNC=1 in the environment may give a more helpful backtrace.");
  }
#endif
#endif

#ifdef MOZ_WIDGET_QT
  
  
  
  if (!PR_GetEnv("MOZ_X_SYNC")) {
    fprintf(stderr, "XError: %s\n", notes.get());
    return 0; 
  }
#endif

  NS_RUNTIMEABORT(notes.get());
  return 0; 
}
}

void
InstallX11ErrorHandler()
{
  XSetErrorHandler(X11Error);

  Display *display = mozilla::DefaultXDisplay();
  NS_ASSERTION(display, "No X display");
  if (PR_GetEnv("MOZ_X_SYNC")) {
    XSynchronize(display, True);
  }
}
