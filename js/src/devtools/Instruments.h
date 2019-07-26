



#ifndef devtools_Instruments_h
#define devtools_Instruments_h

#ifdef __APPLE__

#include <unistd.h>

namespace Instruments {

bool Start(pid_t pid);
void Pause();
bool Resume();
void Stop(const char* profileName);

}

#endif 

#endif 
