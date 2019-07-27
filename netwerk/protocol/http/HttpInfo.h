



#ifndef nsHttpInfo__
#define nsHttpInfo__

#include "nsTArrayForwardDeclare.h"

namespace mozilla {
namespace net {

struct HttpRetParams;

class HttpInfo
{
public:
    
    static void GetHttpConnectionData(nsTArray<HttpRetParams> *);
};

} 
} 

#endif 
