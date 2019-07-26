



#ifndef nsHttpInfo__
#define nsHttpInfo__

template<class T> class nsTArray;

namespace mozilla {
namespace net {

struct HttpRetParams;

class HttpInfo
{
public:
    
    static void GetHttpConnectionData(nsTArray<HttpRetParams> *);
};

} } 

#endif 
