












































#include "nsMemory.h"
#include "nsIServiceManager.h"
#include "nsIProxyObjectManager.h"
#include "nsSupportsArray.h"
#include "nsThreadUtils.h"

#include "nsConsoleService.h"
#include "nsConsoleMessage.h"
#include "nsIClassInfoImpl.h"

NS_IMPL_THREADSAFE_ADDREF(nsConsoleService)
NS_IMPL_THREADSAFE_RELEASE(nsConsoleService)
NS_IMPL_QUERY_INTERFACE1_CI(nsConsoleService, nsIConsoleService)
NS_IMPL_CI_INTERFACE_GETTER1(nsConsoleService, nsIConsoleService)

nsConsoleService::nsConsoleService()
    : mMessages(nsnull), mCurrent(0), mFull(PR_FALSE), mListening(PR_FALSE), mLock(nsnull)
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
    if (mLock)
        PR_DestroyLock(mLock);
}

nsresult
nsConsoleService::Init()
{
    mMessages = (nsIConsoleMessage **)
        nsMemory::Alloc(mBufferSize * sizeof(nsIConsoleMessage *));
    if (!mMessages)
        return NS_ERROR_OUT_OF_MEMORY;

    
    memset(mMessages, 0, mBufferSize * sizeof(nsIConsoleMessage *));

    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

static PRBool PR_CALLBACK snapshot_enum_func(nsHashKey *key, void *data, void* closure)
{
    nsISupportsArray *array = (nsISupportsArray *)closure;

    
    array->AppendElement((nsISupports*)data);
    return PR_TRUE;
}


NS_IMETHODIMP
nsConsoleService::LogMessage(nsIConsoleMessage *message)
{
    if (message == nsnull)
        return NS_ERROR_INVALID_ARG;

    nsSupportsArray listenersSnapshot;
    nsIConsoleMessage *retiredMessage;

    NS_ADDREF(message); 

    



    {
        nsAutoLock lock(mLock);

        




        retiredMessage = mMessages[mCurrent];
        
        mMessages[mCurrent++] = message;
        if (mCurrent == mBufferSize) {
            mCurrent = 0; 
            mFull = PR_TRUE;
        }

        



        mListeners.Enumerate(snapshot_enum_func, &listenersSnapshot);
    }
    if (retiredMessage != nsnull)
        NS_RELEASE(retiredMessage);

    






    nsCOMPtr<nsIConsoleListener> listener;
    nsresult rv;
    nsresult returned_rv;
    PRUint32 snapshotCount;
    rv = listenersSnapshot.Count(&snapshotCount);
    if (NS_FAILED(rv))
        return rv;

    {
        nsAutoLock lock(mLock);
        if (mListening)
            return NS_OK;
        mListening = PR_TRUE;
    }

    returned_rv = NS_OK;
    for (PRUint32 i = 0; i < snapshotCount; i++) {
        rv = listenersSnapshot.GetElementAt(i, getter_AddRefs(listener));
        if (NS_FAILED(rv)) {
            returned_rv = rv;
            break; 
        }
        listener->Observe(message);
    }
    
    {
        nsAutoLock lock(mLock);
        mListening = PR_FALSE;
    }

    return returned_rv;
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

    



    nsAutoLock lock(mLock);

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
        nsAutoLock lock(mLock);
        nsISupportsKey key(listener);

        








        mListeners.Put(&key, proxiedListener);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsConsoleService::UnregisterListener(nsIConsoleListener *listener) {
    nsAutoLock lock(mLock);

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
    


    nsAutoLock lock(mLock);

    mCurrent = 0;
    mFull = PR_FALSE;

    


    for (PRUint32 i = 0; i < mBufferSize && mMessages[i] != nsnull; i++)
        NS_RELEASE(mMessages[i]);

    return NS_OK;
}
