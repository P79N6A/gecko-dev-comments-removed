




#include "nsGDKErrorHandler.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <errno.h>

#include "nsX11ErrorHandler.h"

#include "prenv.h"









static void
GdkErrorHandler(const gchar *log_domain, GLogLevelFlags log_level,
                const gchar *message,  gpointer user_data)
{
  if (strstr(message, "X Window System error")) {
    XErrorEvent event;
    nsDependentCString buffer(message);
    char *endptr;

    


    NS_NAMED_LITERAL_CSTRING(serialString, "(Details: serial ");
    int32_t start = buffer.Find(serialString);
    if (start == kNotFound)
      NS_RUNTIMEABORT(message);

    start += serialString.Length();
    errno = 0;
    event.serial = strtol(buffer.BeginReading() + start, &endptr, 10);
    if (errno)
      NS_RUNTIMEABORT(message);

    NS_NAMED_LITERAL_CSTRING(errorCodeString, " error_code ");    
    if (!StringBeginsWith(Substring(endptr, buffer.EndReading()), errorCodeString))
      NS_RUNTIMEABORT(message);

    errno = 0;
    event.error_code = strtol(endptr + errorCodeString.Length(), &endptr, 10);
    if (errno)
      NS_RUNTIMEABORT(message);

    NS_NAMED_LITERAL_CSTRING(requestCodeString, " request_code ");
    if (!StringBeginsWith(Substring(endptr, buffer.EndReading()), requestCodeString))
      NS_RUNTIMEABORT(message);

    errno = 0;
    event.request_code = strtol(endptr + requestCodeString.Length(), &endptr, 10);
    if (errno)
      NS_RUNTIMEABORT(message);

    NS_NAMED_LITERAL_CSTRING(minorCodeString, " minor_code ");
    start = buffer.Find(minorCodeString, endptr - buffer.BeginReading());
    if (!start)
      NS_RUNTIMEABORT(message);

    errno = 0;
    event.minor_code = strtol(buffer.BeginReading() + start + minorCodeString.Length(), nullptr, 10);
    if (errno)
      NS_RUNTIMEABORT(message);

    event.display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    
    event.resourceid = 0;

    X11Error(event.display, &event);
  } else {
    g_log_default_handler(log_domain, log_level, message, user_data);
    NS_RUNTIMEABORT(message);
  }
}

void
InstallGdkErrorHandler()
{
  g_log_set_handler("Gdk",
                    (GLogLevelFlags)(G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
                    GdkErrorHandler,
                    nullptr);
  if (PR_GetEnv("MOZ_X_SYNC")) {
    XSynchronize(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), True);
  }
}
