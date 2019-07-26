









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_SSRC_DATABASE_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_SSRC_DATABASE_H_

#ifndef WEBRTC_NO_STL
    #include <map>
#endif

#include "system_wrappers/interface/static_instance.h"
#include "typedefs.h"

namespace webrtc {
class CriticalSectionWrapper;

class SSRCDatabase
{
public:
    static SSRCDatabase* GetSSRCDatabase();
    static void ReturnSSRCDatabase();

    WebRtc_UWord32 CreateSSRC();
    WebRtc_Word32 RegisterSSRC(const WebRtc_UWord32 ssrc);
    WebRtc_Word32 ReturnSSRC(const WebRtc_UWord32 ssrc);

protected:
    SSRCDatabase();
    virtual ~SSRCDatabase();

    static SSRCDatabase* CreateInstance() { return new SSRCDatabase(); }

private:
    
    
    friend SSRCDatabase* GetStaticInstance<SSRCDatabase>(
        CountOperation count_operation);
    static SSRCDatabase* StaticInstance(CountOperation count_operation);

    WebRtc_UWord32 GenerateRandom();

#ifdef WEBRTC_NO_STL
    int _numberOfSSRC;
    int _sizeOfSSRC;

    WebRtc_UWord32* _ssrcVector;
#else
    std::map<WebRtc_UWord32, WebRtc_UWord32>    _ssrcMap;
#endif

    CriticalSectionWrapper* _critSect;
};
} 

#endif 
