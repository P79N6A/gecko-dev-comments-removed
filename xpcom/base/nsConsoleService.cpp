












































#include "nsMemory.h"
#include "nsIServiceManager.h"
#include "nsIProxyObjectManager.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"

#include "nsConsoleService.h"
#include "nsConsoleMessage.h"
#include "nsIClassInfoImpl.h"

#if defined(ANDROID)
#include <android/log.h>
#endif

using namespace mozilla;

NS_IMPL_THREADSAFE_ADDREF(nsConsoleService)
NS_IMPL_THREADSAFE_RELEASE(nsConsoleService)
NS_IMPL_CLASSINFO(nsConsoleService, NULL, nsIClassInfo::THREADSAFE | nsIClassInfo::SINGLETON, NS_CONSOLESERVICE_CID)
NS_IMPL_QUERY_INTERFACE1_CI(nsConsoleService, nsIConsoleService)
NS_IMPL_CI_INTERFACE_GETTER1(nsConsoleService, nsIConsoleService)

nsConsoleService::nsConsoleService()
    : mMessages(nsnull), mCurrent(0), mFull(false), mListening(false), mLock("nsConsoleService.mLock")
{
    
    
    
    mBufferSize = 250;
}

nsConsoleService::~nsConsoleService()
{
    PRUint32 i = 0;
    while (i < mBufferSize && mMessages[i] != nsnull) {
        NS_RELEASE(mMessages[i]);
        i++;
    }

#ifdef DEBUG_mccabe
    if (mListeners.Count() != 0) {
        fprintf(stderr, 
            "WARNING - %d console error listeners still registered!\n"
            "More calls to nsIConsoleService::UnregisterListener needed.\n",
            mListeners.Count());
    }
    
#endif

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

    return NS_OK;
}

static bool snapshot_enum_func(nsHashKey *key, void *data, void* closure)
{
    nsCOMArray<nsIConsoleListener> *array =
      reinterpret_cast<nsCOMArray<nsIConsoleListener> *>(closure);

    
    array->AppendObject((nsIConsoleListener*)data);
    return true;
}


NS_IMETHODIMP
nsConsoleService::LogMessage(nsIConsoleMessage *message)
{
    if (message == nsnull)
        return NS_ERROR_INVALID_ARG;

    nsCOMArray<nsIConsoleListener> listenersSnapshot;
    nsIConsoleMessage *retiredMessage;

    NS_ADDREF(message); 

    



    {
        MutexAutoLock lock(mLock);

#if defined(ANDROID)
        {
            nsXPIDLString msg;
            message->GetMessageMoz(getter_Copies(msg));
            __android_log_print(ANDROID_LOG_ERROR, "Gecko *** Console Service *** ",
                        "%s",
                        NS_LossyConvertUTF16toASCII(msg).get());
        }
#endif

        




        retiredMessage = mMessages[mCurrent];
        
        mMessages[mCurrent++] = message;
        if (mCurrent == mBufferSize) {
            mCurrent = 0; 
            mFull = true;
        }

        



        mListeners.Enumerate(snapshot_enum_func, &listenersSnapshot);
    }
    if (retiredMessage != nsnull)
        NS_RELEASE(retiredMessage);

    






    nsCOMPtr<nsIConsoleListener> listener;
    PRInt32 snapshotCount = listenersSnapshot.Count();

    {
        MutexAutoLock lock(mLock);
        if (mListening)
            return NS_OK;
        mListening = true;
    }

    for (PRInt32 i = 0; i < snapshotCount; i++) {
        listenersSnapshot[i]->Observe(message);
    }
    
    {
        MutexAutoLock lock(mLock);
        mListening = false;
    }

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
        *messageArray = nsnull;
        *messages = messageArray;
        *count = 0;
        
        return NS_OK;
    }

    PRUint32 resultSize = mFull ? mBufferSize : mCurrent;
    messageArray =
        (nsIConsoleMessage **)nsMemory::Alloc((sizeof (nsIConsoleMessage *))
                                              * resultSize);

    if (messageArray == nsnull) {
        *messages = nsnull;
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
nsConsoleService::RegisterListener(nsIConsoleListener *listener) {
    nsresult rv;

    






    nsCOMPtr<nsIConsoleListener> proxiedListener;

    rv = GetProxyForListener(listener, getter_AddRefs(proxiedListener));
    if (NS_FAILED(rv))
        return rv;

    {
        MutexAutoLock lock(mLock);
        nsISupportsKey key(listener);

        








        mListeners.Put(&key, proxiedListener);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsConsoleService::UnregisterListener(nsIConsoleListener *listener) {
    MutexAutoLock lock(mLock);

    nsISupportsKey key(listener);
    mListeners.Remove(&key);
    return NS_OK;
}

nsresult 
nsConsoleService::GetProxyForListener(nsIConsoleListener* aListener,
                                      nsIConsoleListener** aProxy)
{
    


    return NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                NS_GET_IID(nsIConsoleListener),
                                aListener,
                                NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                (void**) aProxy);
}

NS_IMETHODIMP
nsConsoleService::Reset()
{
    


    MutexAutoLock lock(mLock);

    mCurrent = 0;
    mFull = false;

    


    for (PRUint32 i = 0; i < mBufferSize && mMessages[i] != nsnull; i++)
        NS_RELEASE(mMessages[i]);

    return NS_OK;
}
