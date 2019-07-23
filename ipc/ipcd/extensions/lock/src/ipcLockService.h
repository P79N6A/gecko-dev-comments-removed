





































#ifndef ipcLockService_h__
#define ipcLockService_h__

#include "ipcILockService.h"
#include "ipcdclient.h"



class ipcLockService : public ipcILockService
                     , public ipcIMessageObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_IPCILOCKSERVICE
    NS_DECL_IPCIMESSAGEOBSERVER

    NS_HIDDEN_(nsresult) Init();

private:
    PRUintn mTPIndex;
};



#endif 
