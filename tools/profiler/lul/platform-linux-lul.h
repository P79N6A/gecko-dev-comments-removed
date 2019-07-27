




#ifndef MOZ_PLATFORM_LINUX_LUL_H
#define MOZ_PLATFORM_LINUX_LUL_H

#include "platform.h"




void
read_procmaps(lul::LUL* aLUL);


void
logging_sink_for_LUL(const char* str);


extern lul::LUL* sLUL;

#endif 
