









#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_CONFIG_H_
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_CONFIG_H_



enum { kAdmMaxIdleTimeProcess = 1000 };
enum { GET_MIC_VOLUME_INTERVAL_MS = 1000 };



#if defined(_WIN32)
#if (_MSC_VER >= 1400)


#define WEBRTC_WINDOWS_CORE_AUDIO_BUILD
#endif
#endif

#endif  

