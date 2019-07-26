



#include "mozilla/net/DashboardTypes.h"

#ifndef nsHttpInfo__
#define nsHttpInfo__

namespace mozilla {
namespace net {

class HttpInfo
{
public:
    
    static void GetHttpConnectionData(nsTArray<HttpRetParams> *);
};

} } 

#endif 
