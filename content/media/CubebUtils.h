





#if !defined(CubebUtils_h_)
#define CubebUtils_h_

#include "cubeb/cubeb.h"
#include "mozilla/dom/AudioChannelBinding.h"

namespace mozilla {
namespace CubebUtils {



void InitLibrary();



void ShutdownLibrary();


uint32_t MaxNumberOfChannels();




void InitPreferredSampleRate();


uint32_t PreferredSampleRate();

void PrefChanged(const char* aPref, void* aClosure);
double GetVolumeScale();
bool GetFirstStream();
cubeb* GetCubebContext();
cubeb* GetCubebContextUnlocked();
uint32_t GetCubebLatency();
bool CubebLatencyPrefSet();
#if defined(__ANDROID__) && defined(MOZ_B2G)
cubeb_stream_type ConvertChannelToCubebType(dom::AudioChannel aChannel);
#endif

} 
} 

#endif 
