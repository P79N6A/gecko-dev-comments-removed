





































#include "nsMaemoNetworkManager.h"
#include "mozilla/ReentrantMonitor.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <conic/conicconnection.h>
#include <conic/conicconnectionevent.h>
#include <conicstatisticsevent.h>

#include <stdio.h>
#include <unistd.h>

#include "nsIOService.h"
#include "nsIObserverService.h"
#include "nsIOService.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"

#include "nsINetworkLinkService.h"

enum InternalState
{
  InternalState_Invalid = -1,
  InternalState_Disconnected,
  InternalState_Connected
};

static InternalState gInternalState = InternalState_Invalid;
static ConIcConnection* gConnection = nsnull;
static bool gConnectionCallbackInvoked = false;

using namespace mozilla;

static ReentrantMonitor* gReentrantMonitor = nsnull;

static void NotifyNetworkLinkObservers()
{
  nsCOMPtr<nsIIOService> ioService = do_GetService("@mozilla.org/network/io-service;1");
  if (!ioService)
    return;

  ioService->SetOffline(gInternalState != InternalState_Connected);
}

static void
connection_event_callback(ConIcConnection *aConnection,
                          ConIcConnectionEvent *aEvent,
                          gpointer aUser_data)
{
  ConIcConnectionStatus status = con_ic_connection_event_get_status(aEvent);
  {
    ReentrantMonitorAutoEnter mon(*gReentrantMonitor);

    
    gInternalState = (CON_IC_STATUS_CONNECTED == status ?
                     InternalState_Connected : InternalState_Disconnected);

    gConnectionCallbackInvoked = PR_TRUE;
    mon.Notify();
  }

  NotifyNetworkLinkObservers();
}

bool
nsMaemoNetworkManager::OpenConnectionSync()
{
  if (NS_IsMainThread() || !gConnection)
    return PR_FALSE;

  
  
  
  ReentrantMonitorAutoEnter mon(*gReentrantMonitor);

  gConnectionCallbackInvoked = PR_FALSE;

  if (!con_ic_connection_connect(gConnection,
                                 CON_IC_CONNECT_FLAG_NONE))
    g_error("openConnectionSync: Error while connecting. %p \n",
            (void*) PR_GetCurrentThread());

  while (!gConnectionCallbackInvoked)
    mon.Wait();

  if (gInternalState == InternalState_Connected)
    return PR_TRUE;

  return PR_FALSE;
}

void
nsMaemoNetworkManager::CloseConnection()
{
  if (gConnection)
    con_ic_connection_disconnect(gConnection);
}

bool
nsMaemoNetworkManager::IsConnected()
{
  return gInternalState == InternalState_Connected;
}

bool
nsMaemoNetworkManager::GetLinkStatusKnown()
{
  return gInternalState != InternalState_Invalid;
}

bool
nsMaemoNetworkManager::Startup()
{
  if (gConnection)
    return PR_TRUE;

  gReentrantMonitor = new ReentrantMonitor("MaemoAutodialer");
  if (!gReentrantMonitor)
    return PR_FALSE;

  DBusError error;
  dbus_error_init(&error);

  DBusConnection* dbusConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  NS_ASSERTION(dbusConnection, "Error when connecting to the session bus");

  dbus_connection_setup_with_g_main(dbusConnection, nsnull);

  
  gConnection = con_ic_connection_new();
  NS_ASSERTION(gConnection, "Error when creating connection");
  if (!gConnection) {
    delete gReentrantMonitor;
    gReentrantMonitor = nsnull;
    return PR_FALSE;
  }

  g_signal_connect(G_OBJECT(gConnection),
                   "connection-event",
                   G_CALLBACK(connection_event_callback),
                   nsnull);
  
  g_object_set(G_OBJECT(gConnection),
               "automatic-connection-events",
               PR_TRUE,
               nsnull);
  return PR_TRUE;
}

void
nsMaemoNetworkManager::Shutdown()
{
  gConnection = nsnull;

  if (gReentrantMonitor) {
    
    ReentrantMonitorAutoEnter mon(*gReentrantMonitor);
    gInternalState = InternalState_Invalid;    
    mon.Notify();
  }
  
  
  
}

