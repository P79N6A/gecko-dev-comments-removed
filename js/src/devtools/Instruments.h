



#ifndef Instruments_h__
#define Instruments_h__

#ifdef __APPLE__

namespace Instruments {

bool Start();
void Pause();
bool Resume();
void Stop(const char* profileName);

}

#endif 

#endif 
