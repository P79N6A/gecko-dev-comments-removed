



#ifndef BreakpadLogging_h
#define BreakpadLogging_h

#include <ostream>

namespace mozilla {


extern std::ostream gNullStream;

} 


#define BPLOG_INFO_STREAM mozilla::gNullStream

#endif 
