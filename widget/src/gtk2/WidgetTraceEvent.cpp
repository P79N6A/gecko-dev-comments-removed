




































#include "mozilla/WidgetTraceEvent.h"

#include <glib.h>
#include <mozilla/CondVar.h>
#include <mozilla/Mutex.h>
#include <stdio.h>

using mozilla::CondVar;
using mozilla::Mutex;
using mozilla::MutexAutoLock;

namespace {

Mutex* sMutex = NULL;
CondVar* sCondVar = NULL;
bool sTracerProcessed = false;


gboolean TracerCallback(gpointer data)
{
  MutexAutoLock lock(*sMutex);
  NS_ABORT_IF_FALSE(!sTracerProcessed, "Tracer synchronization state is wrong");
  sTracerProcessed = true;
  sCondVar->Notify();
  return FALSE;
}

} 

namespace mozilla {

bool InitWidgetTracing()
{
  sMutex = new Mutex("Event tracer thread mutex");
  sCondVar = new CondVar(*sMutex, "Event tracer thread condvar");
  return sMutex && sCondVar;
}

void CleanUpWidgetTracing()
{
  delete sMutex;
  delete sCondVar;
  sMutex = NULL;
  sCondVar = NULL;
}


bool FireAndWaitForTracerEvent()
{
  NS_ABORT_IF_FALSE(sMutex && sCondVar, "Tracing not initialized!");

  
  
  MutexAutoLock lock(*sMutex);
  NS_ABORT_IF_FALSE(!sTracerProcessed, "Tracer synchronization state is wrong");
  g_idle_add_full(G_PRIORITY_DEFAULT,
                  TracerCallback,
                  NULL,
                  NULL);
  while (!sTracerProcessed)
    sCondVar->Wait();
  sTracerProcessed = false;
  return true;
}

}  
