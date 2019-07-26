




#ifndef mozilla_dom_workers_workers_h__
#define mozilla_dom_workers_workers_h__

#include "jsapi.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include "mozilla/StandardInteger.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsStringGlue.h"

#define BEGIN_WORKERS_NAMESPACE \
  namespace mozilla { namespace dom { namespace workers {
#define END_WORKERS_NAMESPACE \
  } /* namespace workers */ } /* namespace dom */ } /* namespace mozilla */
#define USING_WORKERS_NAMESPACE \
  using namespace mozilla::dom::workers;

#define WORKERS_SHUTDOWN_TOPIC "web-workers-shutdown"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_WORKERS_NAMESPACE

class WorkerPrivate;

struct PrivatizableBase
{ };

#ifdef DEBUG
void
AssertIsOnMainThread();
#else
inline void
AssertIsOnMainThread()
{ }
#endif


JSBool
ResolveWorkerClasses(JSContext* aCx, JSHandleObject aObj, JSHandleId aId, unsigned aFlags,
                     JS::MutableHandle<JSObject*> aObjp);

void
CancelWorkersForWindow(JSContext* aCx, nsPIDOMWindow* aWindow);

void
SuspendWorkersForWindow(JSContext* aCx, nsPIDOMWindow* aWindow);

void
ResumeWorkersForWindow(nsIScriptContext* aCx, nsPIDOMWindow* aWindow);

class WorkerTask {
public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WorkerTask)

    virtual ~WorkerTask() { }

    virtual bool RunTask(JSContext* aCx) = 0;
};

class WorkerCrossThreadDispatcher {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WorkerCrossThreadDispatcher)

  WorkerCrossThreadDispatcher(WorkerPrivate* aPrivate) :
    mMutex("WorkerCrossThreadDispatcher"), mPrivate(aPrivate) {}
  void Forget()
  {
    mozilla::MutexAutoLock lock(mMutex);
    mPrivate = nullptr;
  }

  



  bool PostTask(WorkerTask* aTask);

protected:
  friend class WorkerPrivate;

  
  mozilla::Mutex mMutex;
  WorkerPrivate* mPrivate;
};

WorkerCrossThreadDispatcher*
GetWorkerCrossThreadDispatcher(JSContext* aCx, jsval aWorker);


const uint32_t kJSPrincipalsDebugToken = 0x7e2df9d2;

namespace exceptions {


void
ThrowDOMExceptionForNSResult(JSContext* aCx, nsresult aNSResult);

} 

END_WORKERS_NAMESPACE

#endif 
