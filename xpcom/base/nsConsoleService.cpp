











#include "nsMemory.h"
#include "nsIServiceManager.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"

#include "nsConsoleService.h"
#include "nsConsoleMessage.h"
#include "nsIClassInfoImpl.h"
#include "nsThreadUtils.h"

#include "mozilla/Preferences.h"

#if defined(ANDROID)
#include <android/log.h>
#endif
#ifdef XP_WIN
#include <windows.h>
#endif

using namespace mozilla;

NS_IMPL_THREADSAFE_ADDREF(nsConsoleService)
NS_IMPL_THREADSAFE_RELEASE(nsConsoleService)
NS_IMPL_CLASSINFO(nsConsoleService, NULL, nsIClassInfo::THREADSAFE | nsIClassInfo::SINGLETON, NS_CONSOLESERVICE_CID)
NS_IMPL_QUERY_INTERFACE1_CI(nsConsoleService, nsIConsoleService)
NS_IMPL_CI_INTERFACE_GETTER1(nsConsoleService, nsIConsoleService)

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
    while (i < mBufferSize && mMessages[i] != nullptr) {
        NS_RELEASE(mMessages[i]);
        i++;
    }

    if (mMessages)
        nsMemory::Free(mMessages);
}

class AddConsolePrefWatchers : public nsRunnable
{
public:
    AddConsolePrefWatchers(nsConsoleService* aConsole) : mConsole(aConsole) {}

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
    mMessages = (nsIConsoleMessage **)
        nsMemory::Alloc(mBufferSize * sizeof(nsIConsoleMessage *));
    if (!mMessages)
        return NS_ERROR_OUT_OF_MEMORY;

    
    memset(mMessages, 0, mBufferSize * sizeof(nsIConsoleMessage *));

    mListeners.Init();
    NS_DispatchToMainThread(new AddConsolePrefWatchers(this));

    return NS_OK;
}

namespace {

class LogMessageRunnable : public nsRunnable
{
public:
    LogMessageRunnable(nsIConsoleMessage* message, nsConsoleService* service)
        : mMessage(message)
        , mService(service)
    { }

    NS_DECL_NSIRUNNABLE

private:
    nsCOMPtr<nsIConsoleMessage> mMessage;
    nsRefPtr<nsConsoleService> mService;
};

typedef nsCOMArray<nsIConsoleListener> ListenerArrayType;

PLDHashOperator
CollectCurrentListeners(nsISupports* aKey, nsIConsoleListener* aValue,
                        void* closure)
{
    ListenerArrayType* listeners = static_cast<ListenerArrayType*>(closure);
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

    for (int32_t i = 0; i < listeners.Count(); ++i)
        listeners[i]->Observe(mMessage);

    mService->SetDoneDelivering();

    return NS_OK;
}

} 


NS_IMETHODIMP
nsConsoleService::LogMessage(nsIConsoleMessage *message)
{
    return LogMessageWithMode(message, OutputToLog);
}

nsresult
nsConsoleService::LogMessageWithMode(nsIConsoleMessage *message, nsConsoleService::OutputMode outputMode)
{
    if (message == nullptr)
        return NS_ERROR_INVALID_ARG;

    if (!sLoggingEnabled) {
        return NS_OK;
    }

    if (NS_IsMainThread() && mDeliveringMessage) {
        NS_WARNING("Some console listener threw an error while inside itself. Discarding this message");
        return NS_ERROR_FAILURE;
    }

    nsRefPtr<LogMessageRunnable> r;
    nsIConsoleMessage *retiredMessage;

    if (sLoggingBuffered) {
        NS_ADDREF(message); 
    }

    



    {
        MutexAutoLock lock(mLock);

#if defined(ANDROID)
        if (outputMode == OutputToLog)
        {
            nsXPIDLString msg;
            message->GetMessageMoz(getter_Copies(msg));
            __android_log_print(ANDROID_LOG_ERROR, "GeckoConsole",
                        "%s",
                        NS_LossyConvertUTF16toASCII(msg).get());
        }
#endif
#ifdef XP_WIN
        if (IsDebuggerPresent()) {
            nsString msg;
            message->GetMessageMoz(getter_Copies(msg));
            msg.AppendLiteral("\n");
            OutputDebugStringW(msg.get());
        }
#endif

        




        retiredMessage = mMessages[mCurrent];

        if (sLoggingBuffered) {
            mMessages[mCurrent++] = message;
            if (mCurrent == mBufferSize) {
                mCurrent = 0; 
                mFull = true;
            }
        }

        if (mListeners.Count() > 0) {
            r = new LogMessageRunnable(message, this);
        }
    }

    if (retiredMessage != nullptr)
        NS_RELEASE(retiredMessage);

    if (r)
        NS_DispatchToMainThread(r);

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
nsConsoleService::LogStringMessage(const PRUnichar *message)
{
    if (!sLoggingEnabled) {
        return NS_OK;
    }

    nsRefPtr<nsConsoleMessage> msg(new nsConsoleMessage(message));
    return this->LogMessage(msg);
}

NS_IMETHODIMP
nsConsoleService::GetMessageArray(uint32_t *count, nsIConsoleMessage ***messages)
{
    nsIConsoleMessage **messageArray;

    



    MutexAutoLock lock(mLock);

    if (mCurrent == 0 && !mFull) {
        




        messageArray = (nsIConsoleMessage **)
            nsMemory::Alloc(sizeof (nsIConsoleMessage *));
        *messageArray = nullptr;
        *messages = messageArray;
        *count = 0;

        return NS_OK;
    }

    uint32_t resultSize = mFull ? mBufferSize : mCurrent;
    messageArray =
        (nsIConsoleMessage **)nsMemory::Alloc((sizeof (nsIConsoleMessage *))
                                              * resultSize);

    if (messageArray == nullptr) {
        *messages = nullptr;
        *count = 0;
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
    *count = resultSize;
    *messages = messageArray;

    return NS_OK;
}

NS_IMETHODIMP
nsConsoleService::RegisterListener(nsIConsoleListener *listener)
{
    if (!NS_IsMainThread()) {
        NS_ERROR("nsConsoleService::RegisterListener is main thread only.");
        return NS_ERROR_NOT_SAME_THREAD;
    }

    nsCOMPtr<nsISupports> canonical = do_QueryInterface(listener);

    MutexAutoLock lock(mLock);
    if (mListeners.GetWeak(canonical)) {
        
        return NS_ERROR_FAILURE;
    }
    mListeners.Put(canonical, listener);
    return NS_OK;
}

NS_IMETHODIMP
nsConsoleService::UnregisterListener(nsIConsoleListener *listener)
{
    if (!NS_IsMainThread()) {
        NS_ERROR("nsConsoleService::UnregisterListener is main thread only.");
        return NS_ERROR_NOT_SAME_THREAD;
    }

    nsCOMPtr<nsISupports> canonical = do_QueryInterface(listener);

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

    


    for (uint32_t i = 0; i < mBufferSize && mMessages[i] != nullptr; i++)
        NS_RELEASE(mMessages[i]);

    return NS_OK;
}
