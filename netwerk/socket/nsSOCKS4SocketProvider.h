





































#ifndef _NSSOCKS4SOCKETPROVIDER_H_
#define _NSSOCKS4SOCKETPROVIDER_H_

#include "nsISOCKS4SocketProvider.h"


#define NS_SOCKS4SOCKETPROVIDER_CID { 0xf7c9f5f4, 0x4451, 0x41c3, { 0xa2, 0x8a, 0x5b, 0xa2, 0x44, 0x7f, 0xba, 0xce } }

class nsSOCKS4SocketProvider : public nsISOCKS4SocketProvider
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISOCKETPROVIDER
    NS_DECL_NSISOCKS4SOCKETPROVIDER
    
    
    nsSOCKS4SocketProvider();
    virtual ~nsSOCKS4SocketProvider();
    
    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);
    
    nsresult Init();
    
protected:
};

#endif
