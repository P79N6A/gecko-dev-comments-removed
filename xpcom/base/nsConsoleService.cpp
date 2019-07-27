












#include "nsMemory.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"

#include "nsConsoleService.h"
#include "nsConsoleMessage.h"
#include "nsIClassInfoImpl.h"
#include "nsIConsoleListener.h"
#include "nsPrintfCString.h"

#include "mozilla/Preferences.h"

#if defined(ANDROID)
#include <android/log.h>
#include "mozilla/dom/ContentChild.h"
#endif
#ifdef XP_WIN
#include <windows.h>
#endif

#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
using namespace mozilla::tasktracer;
#endif

using namespace mozilla;

NS_IMPL_ADDREF(nsConsoleService)
NS_IMPL_RELEASE(nsConsoleService)
NS_IMPL_CLASSINFO(nsConsoleService, nullptr,
                  nsIClassInfo::THREADSAFE | nsIClassInfo::SINGLETON,
                  NS_CONSOLESERVICE_CID)
NS_IMPL_QUERY_INTERFACE_CI(nsConsoleService, nsIConsoleService)
NS_IMPL_CI_INTERFACE_GETTER(nsConsoleService, nsIConsoleService)

static bool sLoggingEnabled = true;
static bool sLoggingBuffered = true;

nsConsoleService::nsConsoleService()
  : mMessages(nullptr)
  , mCurrent(0)
  , mFull(false)
  , mDeliveringMessage(false)
  , mLock("nsConsoleService.mLock")
{
  
  
  
  mBufferSize = 250;
}

nsConsoleService::~nsConsoleService()
{
  uint32_t i = 0;
  while (i < mBufferSize && mMessages[i]) {
    NS_RELEASE(mMessages[i]);
    i++;
  }

  if (mMessages) {
    free(mMessages);
  }
}

class AddConsolePrefWatchers : public nsRunnable
{
public:
  explicit AddConsolePrefWatchers(nsConsoleService* aConsole) : mConsole(aConsole)
  {
  }

  NS_IMETHOD Run()
  {
    Preferences::AddBoolVarCache(&sLoggingEnabled, "consoleservice.enabled", true);
    Preferences::AddBoolVarCache(&sLoggingBuffered, "consoleservice.buffered", true);
    if (!sLoggingBuffered) {
      mConsole->Reset();
    }
    return NS_OK;
  }

private:
  nsRefPtr<nsConsoleService> mConsole;
};

nsresult
nsConsoleService::Init()
{
  mMessages = (nsIConsoleMessage**)
    moz_xmalloc(mBufferSize * sizeof(nsIConsoleMessage*));
  if (!mMessages) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  memset(mMessages, 0, mBufferSize * sizeof(nsIConsoleMessage*));

  NS_DispatchToMainThread(new AddConsolePrefWatchers(this));

  return NS_OK;
}

namespace {

class LogMessageRunnable : public nsRunnable
{
public:
  LogMessageRunnable(nsIConsoleMessage* aMessage, nsConsoleService* aService)
    : mMessage(aMessage)
    , mService(aService)
  { }

  NS_DECL_NSIRUNNABLE

private:
  nsCOMPtr<nsIConsoleMessage> mMessage;
  nsRefPtr<nsConsoleService> mService;
};

typedef nsCOMArray<nsIConsoleListener> ListenerArrayType;

PLDHashOperator
CollectCurrentListeners(nsISupports* aKey, nsIConsoleListener* aValue,
                        void* aClosure)
{
  ListenerArrayType* listeners = static_cast<ListenerArrayType*>(aClosure);
  listeners->AppendObject(aValue);
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
LogMessageRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  nsCOMArray<nsIConsoleListener> listeners;
  mService->EnumerateListeners(CollectCurrentListeners, &listeners);

  mService->SetIsDelivering();

  for (int32_t i = 0; i < listeners.Count(); ++i) {
    listeners[i]->Observe(mMessage);
  }

  mService->SetDoneDelivering();

  return NS_OK;
}

} 


NS_IMETHODIMP
nsConsoleService::LogMessage(nsIConsoleMessage* aMessage)
{
  return LogMessageWithMode(aMessage, OutputToLog);
}

nsresult
nsConsoleService::LogMessageWithMode(nsIConsoleMessage* aMessage,
                                     nsConsoleService::OutputMode aOutputMode)
{
  if (!aMessage) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!sLoggingEnabled) {
    return NS_OK;
  }

  if (NS_IsMainThread() && mDeliveringMessage) {
    nsCString msg;
    aMessage->ToString(msg);
    NS_WARNING(nsPrintfCString("Reentrancy error: some client attempted "
      "to display a message to the console while in a console listener. "
      "The following message was discarded: \"%s\"", msg.get()).get());
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<LogMessageRunnable> r;
  nsIConsoleMessage* retiredMessage;

  if (sLoggingBuffered) {
    NS_ADDREF(aMessage); 
  }

  



  {
    MutexAutoLock lock(mLock);

#if defined(ANDROID) && !defined(RELEASE_BUILD)
    if (aOutputMode == OutputToLog) {
      nsCString msg;
      aMessage->ToString(msg);

      
      mozilla::dom::ContentChild* child =
          mozilla::dom::ContentChild::GetSingleton();
      nsCString appName;
      if (child) {
        child->GetProcessName(appName);
      } else {
        appName = "GeckoConsole";
      }

      uint32_t logLevel = 0;
      aMessage->GetLogLevel(&logLevel);

      android_LogPriority logPriority = ANDROID_LOG_INFO;
      switch (logLevel) {
        case nsIConsoleMessage::debug:
          logPriority = ANDROID_LOG_DEBUG;
          break;
        case nsIConsoleMessage::info:
          logPriority = ANDROID_LOG_INFO;
          break;
        case nsIConsoleMessage::warn:
          logPriority = ANDROID_LOG_WARN;
          break;
        case nsIConsoleMessage::error:
          logPriority = ANDROID_LOG_ERROR;
          break;
      }

      __android_log_print(logPriority, appName.get(), "%s", msg.get());
    }
#endif
#ifdef XP_WIN
    if (IsDebuggerPresent()) {
      nsString msg;
      aMessage->GetMessageMoz(getter_Copies(msg));
      msg.Append('\n');
      OutputDebugStringW(msg.get());
    }
#endif
#ifdef MOZ_TASK_TRACER
    {
      nsCString msg;
      aMessage->ToString(msg);
      int prefixPos = msg.Find(GetJSLabelPrefix());
      if (prefixPos >= 0) {
        nsDependentCSubstring submsg(msg, prefixPos);
        AddLabel("%s", submsg.BeginReading());
      }
    }
#endif

    




    retiredMessage = mMessages[mCurrent];

    if (sLoggingBuffered) {
      mMessages[mCurrent++] = aMessage;
      if (mCurrent == mBufferSize) {
        mCurrent = 0; 
        mFull = true;
      }
    }

    if (mListeners.Count() > 0) {
      r = new LogMessageRunnable(aMessage, this);
    }
  }

  if (retiredMessage) {
    NS_RELEASE(retiredMessage);
  }

  if (r) {
    NS_DispatchToMainThread(r);
  }

  return NS_OK;
}

void
nsConsoleService::EnumerateListeners(ListenerHash::EnumReadFunction aFunction,
                                     void* aClosure)
{
  MutexAutoLock lock(mLock);
  mListeners.EnumerateRead(aFunction, aClosure);
}

NS_IMETHODIMP
nsConsoleService::LogStringMessage(const char16_t* aMessage)
{
  if (!sLoggingEnabled) {
    return NS_OK;
  }

  nsRefPtr<nsConsoleMessage> msg(new nsConsoleMessage(aMessage));
  return this->LogMessage(msg);
}

NS_IMETHODIMP
nsConsoleService::GetMessageArray(uint32_t* aCount,
                                  nsIConsoleMessage*** aMessages)
{
  nsIConsoleMessage** messageArray;

  



  MutexAutoLock lock(mLock);

  if (mCurrent == 0 && !mFull) {
    




    messageArray = (nsIConsoleMessage**)
      moz_xmalloc(sizeof(nsIConsoleMessage*));
    *messageArray = nullptr;
    *aMessages = messageArray;
    *aCount = 0;

    return NS_OK;
  }

  uint32_t resultSize = mFull ? mBufferSize : mCurrent;
  messageArray =
    (nsIConsoleMessage**)moz_xmalloc((sizeof(nsIConsoleMessage*))
                                         * resultSize);

  if (!messageArray) {
    *aMessages = nullptr;
    *aCount = 0;
    return NS_ERROR_FAILURE;
  }

  uint32_t i;
  if (mFull) {
    for (i = 0; i < mBufferSize; i++) {
      
      
      messageArray[i] = mMessages[(mCurrent + i) % mBufferSize];
      NS_ADDREF(messageArray[i]);
    }
  } else {
    for (i = 0; i < mCurrent; i++) {
      messageArray[i] = mMessages[i];
      NS_ADDREF(messageArray[i]);
    }
  }
  *aCount = resultSize;
  *aMessages = messageArray;

  return NS_OK;
}

NS_IMETHODIMP
nsConsoleService::RegisterListener(nsIConsoleListener* aListener)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsConsoleService::RegisterListener is main thread only.");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  nsCOMPtr<nsISupports> canonical = do_QueryInterface(aListener);

  MutexAutoLock lock(mLock);
  if (mListeners.GetWeak(canonical)) {
    
    return NS_ERROR_FAILURE;
  }
  mListeners.Put(canonical, aListener);
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleService::UnregisterListener(nsIConsoleListener* aListener)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsConsoleService::UnregisterListener is main thread only.");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  nsCOMPtr<nsISupports> canonical = do_QueryInterface(aListener);

  MutexAutoLock lock(mLock);

  if (!mListeners.GetWeak(canonical)) {
    
    return NS_ERROR_FAILURE;
  }
  mListeners.Remove(canonical);
  return NS_OK;
}

NS_IMETHODIMP
nsConsoleService::Reset()
{
  


  MutexAutoLock lock(mLock);

  mCurrent = 0;
  mFull = false;

  


  for (uint32_t i = 0; i < mBufferSize && mMessages[i]; i++) {
    NS_RELEASE(mMessages[i]);
  }

  return NS_OK;
}
