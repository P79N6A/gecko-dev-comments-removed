





#ifndef vtunewrapper_h
#define vtunewrapper_h

#include "vtune/jitprofiling.h"

inline bool
IsVTuneProfilingActive()
{
    return (iJIT_IsProfilingActive() == iJIT_SAMPLING_ON);
}

#endif 
