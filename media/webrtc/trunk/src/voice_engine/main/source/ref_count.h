









#ifndef WEBRTC_VOICE_ENGINE_REF_COUNT_H
#define WEBRTC_VOICE_ENGINE_REF_COUNT_H

namespace webrtc {
class CriticalSectionWrapper;

namespace voe {

class RefCount
{
public:
    RefCount();
    ~RefCount();
    RefCount& operator++(int);
    RefCount& operator--(int);
    void Reset();
    int GetCount() const;
private:
    volatile int _count;
    CriticalSectionWrapper& _crit;
};

}  

}  
#endif    
