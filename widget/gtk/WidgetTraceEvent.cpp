



#include "mozilla/WidgetTraceEvent.h"

#include <glib.h>
#include <mozilla/CondVar.h>
#include <mozilla/Mutex.h>
#include <stdio.h>

using mozilla::CondVar;
using mozilla::Mutex;
using mozilla::MutexAutoLock;

namespace {

Mutex* sMutex = nullptr;
CondVar* sCondVar = nullptr;
bool sTracerProcessed = false;


gboolean TracerCallback(gpointer data)
{
  mozilla::SignalTracerThread();
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
  sMutex = nullptr;
  sCondVar = nullptr;
}


bool FireAndWaitForTracerEvent()
{
  MOZ_ASSERT(sMutex && sCondVar, "Tracing not initialized!");

  
  
  MutexAutoLock lock(*sMutex);
  MOZ_ASSERT(!sTracerProcessed, "Tracer synchronization state is wrong");
  g_idle_add_full(G_PRIORITY_DEFAULT,
                  TracerCallback,
                  nullptr,
                  nullptr);
  while (!sTracerProcessed)
    sCondVar->Wait();
  sTracerProcessed = false;
  return true;
}

void SignalTracerThread()
{
  if (!sMutex || !sCondVar)
    return;
  MutexAutoLock lock(*sMutex);
  if (!sTracerProcessed) {
    sTracerProcessed = true;
    sCondVar->Notify();
  }
}

}  
