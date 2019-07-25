









#include "critical_section_wrapper.h"
#include "ref_count.h"

namespace webrtc {

namespace voe {

RefCount::RefCount() :
    _count(0),
    _crit(*CriticalSectionWrapper::CreateCriticalSection())
{
}

RefCount::~RefCount()
{
    delete &_crit;
}

RefCount&
RefCount::operator++(int)
{
    CriticalSectionScoped lock(&_crit);
    _count++;
    return *this;
}
    
RefCount&
RefCount::operator--(int)
{
    CriticalSectionScoped lock(&_crit);
    _count--;
    return *this;
}
  
void 
RefCount::Reset()
{
    CriticalSectionScoped lock(&_crit);
    _count = 0;
}

int 
RefCount::GetCount() const
{
    return _count;
}

}  

}  
