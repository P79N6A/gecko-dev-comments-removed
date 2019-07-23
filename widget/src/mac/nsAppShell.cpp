











































#include "nsAppShell.h"
#include "nsIToolkit.h"
#include "nsToolkit.h"
#include "nsMacMessagePump.h"

#include <Carbon/Carbon.h>

enum {
  kEventClassMoz = 'MOZZ',
  kEventMozNull  = 0,
};



NS_IMETHODIMP
nsAppShell::ResumeNative(void)
{
  nsresult retval = nsBaseAppShell::ResumeNative();
  if (NS_SUCCEEDED(retval) && (mSuspendNativeCount == 0))
    ScheduleNativeEventCallback();
  return retval;
}

nsAppShell::nsAppShell()
: mCFRunLoop(NULL)
, mCFRunLoopSource(NULL)
, mRunningEventLoop(PR_FALSE)
{
}

nsAppShell::~nsAppShell()
{
  if (mCFRunLoopSource) {
    ::CFRunLoopRemoveSource(mCFRunLoop, mCFRunLoopSource,
                            kCFRunLoopCommonModes);
    ::CFRelease(mCFRunLoopSource);
  }

  if (mCFRunLoop)
    ::CFRelease(mCFRunLoop);
}







nsresult
nsAppShell::Init()
{
  
  

  nsresult rv = NS_GetCurrentToolkit(getter_AddRefs(mToolkit));
  if (NS_FAILED(rv))
    return rv;

  nsIToolkit *toolkit = mToolkit.get();
  mMacPump = new nsMacMessagePump(NS_STATIC_CAST(nsToolkit*, toolkit));
  if (!mMacPump.get())
    return NS_ERROR_OUT_OF_MEMORY;

  
  

  
  mCFRunLoop = (CFRunLoopRef)::GetCFRunLoopFromEventLoop(::GetMainEventLoop());
  NS_ENSURE_STATE(mCFRunLoop);
  ::CFRetain(mCFRunLoop);

  CFRunLoopSourceContext context;
  bzero(&context, sizeof(context));
  
  context.info = this;
  context.perform = ProcessGeckoEvents;
  
  mCFRunLoopSource = ::CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
  NS_ENSURE_STATE(mCFRunLoopSource);

  ::CFRunLoopAddSource(mCFRunLoop, mCFRunLoopSource, kCFRunLoopCommonModes);

  return nsBaseAppShell::Init();
}








void
nsAppShell::ScheduleNativeEventCallback()
{
  

  NS_ADDREF_THIS();
  ::CFRunLoopSourceSignal(mCFRunLoopSource);
  ::CFRunLoopWakeUp(mCFRunLoop);
}









PRBool
nsAppShell::ProcessNextNativeEvent(PRBool aMayWait)
{
  PRBool eventProcessed = PR_FALSE;
  PRBool wasRunningEventLoop = mRunningEventLoop;

  mRunningEventLoop = aMayWait;
  EventTimeout waitUntil = kEventDurationNoWait;
  if (aMayWait)
    waitUntil = kEventDurationForever;

  do {
    EventRef carbonEvent;
    OSStatus err = ::ReceiveNextEvent(0, nsnull, waitUntil, PR_TRUE,
                                      &carbonEvent);
    if (err == noErr) {
      ::SendEventToEventTarget(carbonEvent, ::GetEventDispatcherTarget());
      ::ReleaseEvent(carbonEvent);
      eventProcessed = PR_TRUE;
    }
  } while (mRunningEventLoop);

  mRunningEventLoop = wasRunningEventLoop;

  return eventProcessed;
}











void
nsAppShell::ProcessGeckoEvents(void* aInfo)
{
  nsAppShell* self = NS_STATIC_CAST(nsAppShell*, aInfo);

  if (self->mRunningEventLoop) {
    self->mRunningEventLoop = PR_FALSE;

    
    
    EventRef bogusEvent;
    OSStatus err = ::CreateEvent(nsnull, kEventClassMoz, kEventMozNull, 0,
                                 kEventAttributeNone, &bogusEvent);
    if (err == noErr) {
      ::PostEventToQueue(::GetMainEventQueue(), bogusEvent,
                         kEventPriorityStandard);
      ::ReleaseEvent(bogusEvent);
    }
  }

  if (self->mSuspendNativeCount <= 0)
    self->NativeEventCallback();

  NS_RELEASE(self);
}
