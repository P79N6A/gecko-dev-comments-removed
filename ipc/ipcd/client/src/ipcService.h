




































#ifndef ipcService_h__
#define ipcService_h__

#include "ipcIService.h"
#include "ipcdclient.h"

class ipcService : public ipcIService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_IPCISERVICE
};

#endif
