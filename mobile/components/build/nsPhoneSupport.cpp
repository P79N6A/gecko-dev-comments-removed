








































#if (MOZ_PLATFORM_MAEMO == 5)
#include <dbus/dbus.h>
#endif

#ifdef MOZ_WIDGET_QT
#include <QtGui/QApplication>
#include <QtGui/QWidget>
#endif

#include "nsPhoneSupport.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS1(nsPhoneSupport, nsIPhoneSupport)

NS_IMETHODIMP
nsPhoneSupport::SwitchTask()
{
#if (MOZ_PLATFORM_MAEMO == 5)
  DBusError error;
  dbus_error_init(&error);

  DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &error);

  DBusMessage *msg = dbus_message_new_signal("/com/nokia/hildon_desktop",
                                             "com.nokia.hildon_desktop",
                                             "exit_app_view");

  if (msg) {
      dbus_connection_send(conn, msg, NULL);
      dbus_message_unref(msg);
      dbus_connection_flush(conn);
  }
  return NS_OK;
#elif MOZ_WIDGET_QT
  QWidget * window = QApplication::activeWindow();
  if (window)
      window->showMinimized();
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}
