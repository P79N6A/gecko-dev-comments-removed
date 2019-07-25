



































#include "nsHttpActivityDistributor.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsAutoLock.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"

class nsHttpActivityEvent : public nsRunnable
{
public:
    nsHttpActivityEvent(nsISupports *aHttpChannel,
                        PRUint32 aActivityType,
                        PRUint32 aActivitySubtype,
                        PRTime aTimestamp,
                        PRUint64 aExtraSizeData,
                        const nsACString & aExtraStringData,
                        nsCOMArray<nsIHttpActivityObserver> *aObservers)
        : mHttpChannel(aHttpChannel)
        , mActivityType(aActivityType)
        , mActivitySubtype(aActivitySubtype)
        , mTimestamp(aTimestamp)
        , mExtraSizeData(aExtraSizeData)
        , mExtraStringData(aExtraStringData)
        , mObservers(*aObservers)
    {
    }

    NS_IMETHOD Run()
    {
        for (PRInt32 i = 0 ; i < mObservers.Count() ; i++)
            mObservers[i]->ObserveActivity(mHttpChannel, mActivityType,
                                           mActivitySubtype, mTimestamp,
                                           mExtraSizeData, mExtraStringData);
        return NS_OK;
    }

private:
    virtual ~nsHttpActivityEvent()
    {
    }

    nsCOMPtr<nsISupports> mHttpChannel;
    PRUint32 mActivityType;
    PRUint32 mActivitySubtype;
    PRTime mTimestamp;
    PRUint64 mExtraSizeData;
    nsCString mExtraStringData;

    nsCOMArray<nsIHttpActivityObserver> mObservers;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsHttpActivityDistributor,
                              nsIHttpActivityDistributor,
                              nsIHttpActivityObserver)

nsHttpActivityDistributor::nsHttpActivityDistributor()
    : mLock(nsnull)
{
}

nsHttpActivityDistributor::~nsHttpActivityDistributor()
{
    if (mLock)
        PR_DestroyLock(mLock);
}

NS_IMETHODIMP
nsHttpActivityDistributor::ObserveActivity(nsISupports *aHttpChannel,
                                           PRUint32 aActivityType,
                                           PRUint32 aActivitySubtype,
                                           PRTime aTimestamp,
                                           PRUint64 aExtraSizeData,
                                           const nsACString & aExtraStringData)
{
    nsRefPtr<nsIRunnable> event;
    {
        nsAutoLock lock(mLock);

        if (!mObservers.Count())
            return NS_OK;

        event = new nsHttpActivityEvent(aHttpChannel, aActivityType,
                                        aActivitySubtype, aTimestamp,
                                        aExtraSizeData, aExtraStringData,
                                        &mObservers);
    }
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);
    return NS_DispatchToMainThread(event);
}

NS_IMETHODIMP
nsHttpActivityDistributor::GetIsActive(PRBool *isActive)
{
    NS_ENSURE_ARG_POINTER(isActive);
    nsAutoLock lock(mLock);
    *isActive = !!mObservers.Count();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpActivityDistributor::AddObserver(nsIHttpActivityObserver *aObserver)
{
    nsAutoLock lock(mLock);

    if (!mObservers.AppendObject(aObserver))
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpActivityDistributor::RemoveObserver(nsIHttpActivityObserver *aObserver)
{
    nsAutoLock lock(mLock);

    if (!mObservers.RemoveObject(aObserver))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

nsresult
nsHttpActivityDistributor::Init()
{
    NS_ENSURE_TRUE(!mLock, NS_ERROR_ALREADY_INITIALIZED);

    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}
