



#ifndef UseCounter_h_
#define UseCounter_h_

namespace mozilla {

enum UseCounter {
  eUseCounter_UNKNOWN = -1,
#define USE_COUNTER_DOM_METHOD(interface_, name_) \
    eUseCounter_##interface_##_##name_,
#define USE_COUNTER_DOM_ATTRIBUTE(interface_, name_) \
    eUseCounter_##interface_##_##name_,
#define USE_COUNTER_CSS_PROPERTY(name_, id_) \
    eUseCounter_property_##id_,
#include "mozilla/dom/UseCounterList.h"
#undef USE_COUNTER_DOM_METHOD
#undef USE_COUNTER_DOM_ATTRIBUTE
#undef USE_COUNTER_CSS_PROPERTY
  eUseCounter_Count
};

}

#endif
