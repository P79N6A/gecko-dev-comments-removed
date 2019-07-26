



#include "nsHttpHandler.h"
#include "HttpInfo.h"


void
mozilla::net::HttpInfo::
GetHttpConnectionData(nsTArray<HttpRetParams>* args)
{
    if (gHttpHandler)
        gHttpHandler->ConnMgr()->GetConnectionData(args);
}
