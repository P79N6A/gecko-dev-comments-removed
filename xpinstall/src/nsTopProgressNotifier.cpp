








































#include "nsTopProgressNotifier.h"

nsTopProgressListener::nsTopProgressListener()
{
    mListeners = new nsVoidArray();
    mActive = 0;
    mLock = PR_NewLock();
}

nsTopProgressListener::~nsTopProgressListener()
{
    if (mLock) PR_Lock(mLock);

    if (mListeners)
    {
        PRInt32 i=0;
        for (; i < mListeners->Count(); i++)
        {
            nsIXPIListener* element = (nsIXPIListener*)mListeners->ElementAt(i);
            NS_IF_RELEASE(element);
        }

        mListeners->Clear();
        delete (mListeners);
    }

    if (mLock)
    {
        PR_Unlock(mLock);
        PR_DestroyLock(mLock);
    }
}


NS_IMPL_THREADSAFE_ISUPPORTS1(nsTopProgressListener, nsIXPIListener)


long
nsTopProgressListener::RegisterListener(nsIXPIListener * newListener)
{
    if (mLock) PR_Lock(mLock);
    NS_IF_ADDREF( newListener );
    long retval = mListeners->AppendElement( newListener );
    if (mLock) PR_Unlock(mLock);
    return retval;
}


void
nsTopProgressListener::UnregisterListener(long id)
{
    if (mLock) PR_Lock(mLock);
    if (id < mListeners->Count())
    {
        nsIXPIListener *item = (nsIXPIListener*)mListeners->ElementAt(id);
        mListeners->ReplaceElementAt(nsnull, id);
        NS_IF_RELEASE(item);
    }
    if (mLock) PR_Unlock(mLock);
}



NS_IMETHODIMP
nsTopProgressListener::OnInstallStart(const PRUnichar *URL)
{
    if (mActive)
        mActive->OnInstallStart(URL);

    if (mListeners)
    {
        PRInt32 i=0;
        for (; i < mListeners->Count(); i++)
        {
            nsIXPIListener* element = (nsIXPIListener*)mListeners->ElementAt(i);
            if (element != NULL)
                element->OnInstallStart(URL);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsTopProgressListener::OnPackageNameSet(const PRUnichar *URL, const PRUnichar* UIPackageName, const PRUnichar* aVersion)
{
    if (mActive)
        mActive->OnPackageNameSet(URL, UIPackageName, aVersion);

    if (mListeners)
    {
        PRInt32 i=0;
        for (; i < mListeners->Count(); i++)
        {
            nsIXPIListener* element = (nsIXPIListener*)mListeners->ElementAt(i);
            if (element != NULL)
                element->OnPackageNameSet(URL, UIPackageName, aVersion);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsTopProgressListener::OnItemScheduled( const PRUnichar* message )
{
    long rv = 0;

    if (mActive)
        mActive->OnItemScheduled( message );

    if (mListeners)
    {
        PRInt32 i=0;
        for (; i < mListeners->Count(); i++)
        {
            nsIXPIListener* element = (nsIXPIListener*)mListeners->ElementAt(i);
            if (element != NULL)
                element->OnItemScheduled( message );
        }
    }

    return rv;
}

NS_IMETHODIMP
nsTopProgressListener::OnFinalizeProgress( const PRUnichar* message, PRInt32 itemNum, PRInt32 totNum )
{
    if (mActive)
        mActive->OnFinalizeProgress( message, itemNum, totNum );

    if (mListeners)
    {
        PRInt32 i=0;
        for (; i < mListeners->Count(); i++)
        {
            nsIXPIListener* element = (nsIXPIListener*)mListeners->ElementAt(i);
            if (element != NULL)
                element->OnFinalizeProgress( message, itemNum, totNum );
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsTopProgressListener::OnInstallDone(const PRUnichar *URL, PRInt32 status)
{
    if (mActive)
        mActive->OnInstallDone(URL, status);

    if (mListeners)
    {
        PRInt32 i=0;
        for (; i < mListeners->Count(); i++)
        {
            nsIXPIListener* element = (nsIXPIListener*)mListeners->ElementAt(i);
            if (element != NULL)
                element->OnInstallDone(URL,status);
        }
    }
   return NS_OK;
}

NS_IMETHODIMP
nsTopProgressListener::OnLogComment(const PRUnichar* comment)
{
    if (mActive)
        mActive->OnLogComment(comment);

   if (mListeners)
    {
        PRInt32 i=0;
        for (; i < mListeners->Count(); i++)
        {
            nsIXPIListener* element = (nsIXPIListener*)mListeners->ElementAt(i);
            if (element != NULL)
                element->OnLogComment(comment);
        }
    }
   return NS_OK;
}

