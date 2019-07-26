









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_SSRC_DATABASE_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_SSRC_DATABASE_H_

#ifndef WEBRTC_NO_STL
#include <map>
#endif

#include "webrtc/system_wrappers/interface/static_instance.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class CriticalSectionWrapper;

class SSRCDatabase
{
public:
    static SSRCDatabase* GetSSRCDatabase();
    static void ReturnSSRCDatabase();

    uint32_t CreateSSRC();
    int32_t RegisterSSRC(const uint32_t ssrc);
    int32_t ReturnSSRC(const uint32_t ssrc);

protected:
    SSRCDatabase();
    virtual ~SSRCDatabase();

    static SSRCDatabase* CreateInstance() { return new SSRCDatabase(); }

private:
    
    
    friend SSRCDatabase* GetStaticInstance<SSRCDatabase>(
        CountOperation count_operation);
    static SSRCDatabase* StaticInstance(CountOperation count_operation);

    uint32_t GenerateRandom();

#ifdef WEBRTC_NO_STL
    int _numberOfSSRC;
    int _sizeOfSSRC;

    uint32_t* _ssrcVector;
#else
    std::map<uint32_t, uint32_t>    _ssrcMap;
#endif

    CriticalSectionWrapper* _critSect;
};
}  

#endif 
