



#ifndef devtools_Instruments_h
#define devtools_Instruments_h

#ifdef __APPLE__

namespace Instruments {

bool Start();
void Pause();
bool Resume();
void Stop(const char* profileName);

}

#endif 

#endif 
