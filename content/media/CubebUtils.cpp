





#include <stdint.h>
#include <algorithm>
#include "mozilla/Preferences.h"
#include "CubebUtils.h"
#include "prdtoa.h"

#define PREF_VOLUME_SCALE "media.volume_scale"
#define PREF_CUBEB_LATENCY "media.cubeb_latency_ms"

namespace mozilla {

extern PRLogModuleInfo* gAudioStreamLog;

static const uint32_t CUBEB_NORMAL_LATENCY_MS = 100;

StaticMutex CubebUtils::sMutex;
cubeb* CubebUtils::sCubebContext;
uint32_t CubebUtils::sPreferredSampleRate;
double CubebUtils::sVolumeScale;
uint32_t CubebUtils::sCubebLatency;
bool CubebUtils::sCubebLatencyPrefSet;

 void CubebUtils::PrefChanged(const char* aPref, void* aClosure)
{
  if (strcmp(aPref, PREF_VOLUME_SCALE) == 0) {
    nsAdoptingString value = Preferences::GetString(aPref);
    StaticMutexAutoLock lock(sMutex);
    if (value.IsEmpty()) {
      sVolumeScale = 1.0;
    } else {
      NS_ConvertUTF16toUTF8 utf8(value);
      sVolumeScale = std::max<double>(0, PR_strtod(utf8.get(), nullptr));
    }
  } else if (strcmp(aPref, PREF_CUBEB_LATENCY) == 0) {
    
    
    
    sCubebLatencyPrefSet = Preferences::HasUserValue(aPref);
    uint32_t value = Preferences::GetUint(aPref, CUBEB_NORMAL_LATENCY_MS);
    StaticMutexAutoLock lock(sMutex);
    sCubebLatency = std::min<uint32_t>(std::max<uint32_t>(value, 1), 1000);
  }
}

 bool CubebUtils::GetFirstStream()
{
  static bool sFirstStream = true;

  StaticMutexAutoLock lock(sMutex);
  bool result = sFirstStream;
  sFirstStream = false;
  return result;
}

 double CubebUtils::GetVolumeScale()
{
  StaticMutexAutoLock lock(sMutex);
  return sVolumeScale;
}

 cubeb* CubebUtils::GetCubebContext()
{
  StaticMutexAutoLock lock(sMutex);
  return GetCubebContextUnlocked();
}

 void CubebUtils::InitPreferredSampleRate()
{
  StaticMutexAutoLock lock(sMutex);
  if (sPreferredSampleRate == 0 &&
      cubeb_get_preferred_sample_rate(GetCubebContextUnlocked(),
                                      &sPreferredSampleRate) != CUBEB_OK) {
    sPreferredSampleRate = 44100;
  }
}

 cubeb* CubebUtils::GetCubebContextUnlocked()
{
  sMutex.AssertCurrentThreadOwns();
  if (sCubebContext ||
      cubeb_init(&sCubebContext, "CubebUtils") == CUBEB_OK) {
    return sCubebContext;
  }
  NS_WARNING("cubeb_init failed");
  return nullptr;
}

 uint32_t CubebUtils::GetCubebLatency()
{
  StaticMutexAutoLock lock(sMutex);
  return sCubebLatency;
}

 bool CubebUtils::CubebLatencyPrefSet()
{
  StaticMutexAutoLock lock(sMutex);
  return sCubebLatencyPrefSet;
}

 void CubebUtils::InitLibrary()
{
#ifdef PR_LOGGING
  gAudioStreamLog = PR_NewLogModule("AudioStream");
#endif
  PrefChanged(PREF_VOLUME_SCALE, nullptr);
  Preferences::RegisterCallback(PrefChanged, PREF_VOLUME_SCALE);
  PrefChanged(PREF_CUBEB_LATENCY, nullptr);
  Preferences::RegisterCallback(PrefChanged, PREF_CUBEB_LATENCY);
}

 void CubebUtils::ShutdownLibrary()
{
  Preferences::UnregisterCallback(PrefChanged, PREF_VOLUME_SCALE);
  Preferences::UnregisterCallback(PrefChanged, PREF_CUBEB_LATENCY);

  StaticMutexAutoLock lock(sMutex);
  if (sCubebContext) {
    cubeb_destroy(sCubebContext);
    sCubebContext = nullptr;
  }
}

 int CubebUtils::MaxNumberOfChannels()
{
  cubeb* cubebContext = CubebUtils::GetCubebContext();
  uint32_t maxNumberOfChannels;
  if (cubebContext &&
      cubeb_get_max_channel_count(cubebContext,
                                  &maxNumberOfChannels) == CUBEB_OK) {
    return static_cast<int>(maxNumberOfChannels);
  }

  return 0;
}

 int CubebUtils::PreferredSampleRate()
{
  MOZ_ASSERT(sPreferredSampleRate,
             "sPreferredSampleRate has not been initialized!");
  return sPreferredSampleRate;
}

#if defined(__ANDROID__) && defined(MOZ_B2G)
 cubeb_stream_type CubebUtils::ConvertChannelToCubebType(dom::AudioChannel aChannel)
{
  switch(aChannel) {
    case dom::AudioChannel::Normal:
      return CUBEB_STREAM_TYPE_SYSTEM;
    case dom::AudioChannel::Content:
      return CUBEB_STREAM_TYPE_MUSIC;
    case dom::AudioChannel::Notification:
      return CUBEB_STREAM_TYPE_NOTIFICATION;
    case dom::AudioChannel::Alarm:
      return CUBEB_STREAM_TYPE_ALARM;
    case dom::AudioChannel::Telephony:
      return CUBEB_STREAM_TYPE_VOICE_CALL;
    case dom::AudioChannel::Ringer:
      return CUBEB_STREAM_TYPE_RING;
    case dom::AudioChannel::Publicnotification:
      return CUBEB_STREAM_TYPE_SYSTEM_ENFORCED;
    default:
      NS_ERROR("The value of AudioChannel is invalid");
      return CUBEB_STREAM_TYPE_MAX;
  }
}
#endif

}
