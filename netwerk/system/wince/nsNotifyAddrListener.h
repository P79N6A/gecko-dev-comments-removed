



































#ifndef NSNOTIFYADDRLISTENER_H_
#define NSNOTIFYADDRLISTENER_H_

#include <windows.h>

#include "nsINetworkLinkService.h"
#include "nsCOMPtr.h"


class nsNotifyAddrListener : public nsINetworkLinkService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSINETWORKLINKSERVICE

    nsNotifyAddrListener();
    virtual ~nsNotifyAddrListener();

    nsresult Init(void);

protected:
    HANDLE mConnectionHandle;
};

#endif 
