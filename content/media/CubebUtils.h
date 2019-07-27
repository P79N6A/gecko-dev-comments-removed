





#if !defined(CubebUtils_h_)
#define CubebUtils_h_

#include "cubeb/cubeb.h"
#include "nsAutoRef.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/dom/AudioChannelBinding.h"

template <>
class nsAutoRefTraits<cubeb_stream> : public nsPointerRefTraits<cubeb_stream>
{
public:
  static void Release(cubeb_stream* aStream) { cubeb_stream_destroy(aStream); }
};

namespace mozilla {

class CubebUtils {
public:
  
  
  static void InitLibrary();

  
  
  static void ShutdownLibrary();

  
  static int MaxNumberOfChannels();

  
  
  
  static void InitPreferredSampleRate();
  
  static int PreferredSampleRate();

  static void PrefChanged(const char* aPref, void* aClosure);
  static double GetVolumeScale();
  static bool GetFirstStream();
  static cubeb* GetCubebContext();
  static cubeb* GetCubebContextUnlocked();
  static uint32_t GetCubebLatency();
  static bool CubebLatencyPrefSet();
#if defined(__ANDROID__) && defined(MOZ_B2G)
  static cubeb_stream_type ConvertChannelToCubebType(dom::AudioChannel aChannel);
#endif

private:
  
  static StaticMutex sMutex;
  static cubeb* sCubebContext;

  
  
  static uint32_t sPreferredSampleRate;

  static double sVolumeScale;
  static uint32_t sCubebLatency;
  static bool sCubebLatencyPrefSet;
};
}



#endif 
