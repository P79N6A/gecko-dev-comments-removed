











#include "nsMemory.h"
#include "nsIServiceManager.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"

#include "nsConsoleService.h"
#include "nsConsoleMessage.h"
#include "nsIClassInfoImpl.h"

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
    PRUint32 i = 0;
    while (i < mBufferSize && mMessages[i] != nullptr) {
        NS_RELEASE(mMessages[i]);
        i++;
    }

    if (mMessages)
        nsMemory::Free(mMessages);
}

nsresult
nsConsoleService::Init()
{
    mMessages = (nsIConsoleMessage **)
        nsMemory::Alloc(mBufferSize * sizeof(nsIConsoleMessage *));
    if (!mMessages)
        return NS_ERROR_OUT_OF_MEMORY;

    
    memset(mMessages, 0, mBufferSize * sizeof(nsIConsoleMessage *));

    mListeners.Init();

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

    void AddListener(nsIConsoleListener* listener) {
        mListeners.AppendObject(listener);
    }

    NS_DECL_NSIRUNNABLE

private:
    nsCOMPtr<nsIConsoleMessage> mMessage;
    nsRefPtr<nsConsoleService> mService;
    nsCOMArray<nsIConsoleListener> mListeners;
};

NS_IMETHODIMP
LogMessageRunnable::Run()
{
    MOZ_ASSERT(NS_IsMainThread());

    mService->SetIsDelivering();

    for (PRInt32 i = 0; i < mListeners.Count(); ++i)
        mListeners[i]->Observe(mMessage);

    mService->SetDoneDelivering();

    return NS_OK;
}

PLDHashOperator
CollectCurrentListeners(nsISupports* aKey, nsIConsoleListener* aValue,
                        void* closure)
{
    LogMessageRunnable* r = static_cast<LogMessageRunnable*>(closure);
    r->AddListener(aValue);
    return PL_DHASH_NEXT;
}

} 


NS_IMETHODIMP
nsConsoleService::LogMessage(nsIConsoleMessage *message)
{
    if (message == nullptr)
        return NS_ERROR_INVALID_ARG;

    if (NS_IsMainThread() && mDeliveringMessage) {
        NS_WARNING("Some console listener threw an error while inside itself. Discarding this message");
        return NS_ERROR_FAILURE;
    }

    nsRefPtr<LogMessageRunnable> r = new LogMessageRunnable(message, this);
    nsIConsoleMessage *retiredMessage;

    NS_ADDREF(message); 

    



    {
        MutexAutoLock lock(mLock);

#if defined(ANDROID)
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
        
        mMessages[mCurrent++] = message;
        if (mCurrent == mBufferSize) {
            mCurrent = 0; 
            mFull = true;
        }

        



        mListeners.EnumerateRead(CollectCurrentListeners, r);
    }
    if (retiredMessage != nullptr)
        NS_RELEASE(retiredMessage);

    NS_DispatchToMainThread(r);

    return NS_OK;
}

NS_IMETHODIMP
nsConsoleService::LogStringMessage(const PRUnichar *message)
{
    nsConsoleMessage *msg = new nsConsoleMessage(message);
    return this->LogMessage(msg);
}

NS_IMETHODIMP
nsConsoleService::GetMessageArray(nsIConsoleMessage ***messages, PRUint32 *count)
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

    PRUint32 resultSize = mFull ? mBufferSize : mCurrent;
    messageArray =
        (nsIConsoleMessage **)nsMemory::Alloc((sizeof (nsIConsoleMessage *))
                                              * resultSize);

    if (messageArray == nullptr) {
        *messages = nullptr;
        *count = 0;
        return NS_ERROR_FAILURE;
    }

    PRUint32 i;
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

    


    for (PRUint32 i = 0; i < mBufferSize && mMessages[i] != nullptr; i++)
        NS_RELEASE(mMessages[i]);

    return NS_OK;
}
