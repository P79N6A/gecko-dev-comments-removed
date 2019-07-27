





#ifndef devtools_sharkctl_h
#define devtools_sharkctl_h

#ifdef __APPLE__

#include <mach/mach.h>
#include <stdint.h>

namespace Shark {

bool Start();
void Stop();

} 

#endif 

#endif 
