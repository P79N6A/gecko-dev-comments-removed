






































#include "nsString.h"
#include "nsStubNotifier.h"

extern PRInt32 gInstallStatus;

nsStubListener::nsStubListener( pfnXPIProgress aProgress )
    : m_progress(aProgress)
{
}

nsStubListener::~nsStubListener()
{}

NS_IMPL_ISUPPORTS1(nsStubListener, nsIXPIListener)


NS_IMETHODIMP
nsStubListener::OnInstallStart(const PRUnichar *URL)
{
    
    return NS_OK;
}

NS_IMETHODIMP
nsStubListener::OnPackageNameSet(const PRUnichar *URL, const PRUnichar* UIPackageName, const PRUnichar* aVersion)
{
    
    return NS_OK;
}

NS_IMETHODIMP
nsStubListener::OnItemScheduled(const PRUnichar* message )
{
    if (m_progress)
      {
        m_progress( NS_LossyConvertUTF16toASCII(message).get(), 0, 0 );
      }
    return NS_OK;
}

NS_IMETHODIMP
nsStubListener::OnFinalizeProgress(const PRUnichar* message, PRInt32 itemNum, PRInt32 totNum )
{
    if (m_progress)
      {
        m_progress( NS_LossyConvertUTF16toASCII(message).get(), itemNum, totNum );
      }
    return NS_OK;
}

NS_IMETHODIMP
nsStubListener::OnInstallDone(const PRUnichar *URL, PRInt32 status)
{


    gInstallStatus = status;
    return NS_OK;
}

NS_IMETHODIMP
nsStubListener::OnLogComment(const PRUnichar* comment)
{
    
    return NS_OK;
}
