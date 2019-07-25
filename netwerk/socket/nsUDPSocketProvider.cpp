



































#include "nsUDPSocketProvider.h"

#include "nspr.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUDPSocketProvider, nsISocketProvider)

nsUDPSocketProvider::~nsUDPSocketProvider()
{
}

NS_IMETHODIMP 
nsUDPSocketProvider::NewSocket(PRInt32 aFamily,
                               const char *aHost, 
                               PRInt32 aPort, 
                               const char *aProxyHost, 
                               PRInt32 aProxyPort,
                               PRUint32 aFlags,
                               PRFileDesc * *aFileDesc, 
                               nsISupports **aSecurityInfo)
{
    NS_ENSURE_ARG_POINTER(aFileDesc);
  
    PRFileDesc* udpFD = PR_OpenUDPSocket(aFamily);
    if (!udpFD)
        return NS_ERROR_FAILURE;
  
    *aFileDesc = udpFD;
    return NS_OK;
}

NS_IMETHODIMP 
nsUDPSocketProvider::AddToSocket(PRInt32 aFamily,
                                 const char *aHost,
                                 PRInt32 aPort,
                                 const char *aProxyHost,
                                 PRInt32 aProxyPort,
                                 PRUint32 aFlags,
                                 struct PRFileDesc * aFileDesc,
                                 nsISupports **aSecurityInfo)
{
    
    NS_NOTREACHED("Cannot layer UDP socket on an existing socket");
    return NS_ERROR_UNEXPECTED;
}
