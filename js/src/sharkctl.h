






#ifndef _SHARKCTL_H
#define _SHARKCTL_H

#ifdef __APPLE__

#include <mach/mach.h>
#include <stdint.h>

namespace Shark {

bool Start();
void Stop();

};

#endif

#endif
