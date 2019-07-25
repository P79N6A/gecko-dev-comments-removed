




































#include "mozilla/WidgetTraceEvent.h"

#include <glib.h>
#include <mozilla/CondVar.h>
#include <mozilla/Mutex.h>
#include <stdio.h>

using mozilla::CondVar;
using mozilla::Mutex;
using mozilla::MutexAutoLock;

namespace {

Mutex sMutex("Event tracer thread mutex");
CondVar sCondVar(sMutex, "Event tracer thread condvar");
bool sTracerProcessed = false;


gboolean TracerCallback(gpointer data)
{
  MutexAutoLock lock(sMutex);
  NS_ABORT_IF_FALSE(!sTracerProcessed, "Tracer synchronization state is wrong");
  sTracerProcessed = true;
  sCondVar.Notify();
  return FALSE;
}

} 

namespace mozilla {


bool FireAndWaitForTracerEvent()
{
  
  
  MutexAutoLock lock(sMutex);
  NS_ABORT_IF_FALSE(!sTracerProcessed, "Tracer synchronization state is wrong");
  g_idle_add_full(G_PRIORITY_DEFAULT,
                  TracerCallback,
                  NULL,
                  NULL);
  while (!sTracerProcessed)
    sCondVar.Wait();
  sTracerProcessed = false;
  return true;
}

}  
