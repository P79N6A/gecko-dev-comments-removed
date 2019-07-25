









#ifndef WEBRTC_VOICE_ENGINE_VOE_AUDIO_PROCESSING_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_AUDIO_PROCESSING_IMPL_H

#include "voe_audio_processing.h"

#include "ref_count.h"
#include "shared_data.h"

namespace webrtc {

class VoEAudioProcessingImpl
    : public VoEAudioProcessing,
      public voe::RefCount {
 public:
  virtual int Release();

  virtual int SetNsStatus(bool enable, NsModes mode = kNsUnchanged);

  virtual int GetNsStatus(bool& enabled, NsModes& mode);

  virtual int SetAgcStatus(bool enable, AgcModes mode = kAgcUnchanged);

  virtual int GetAgcStatus(bool& enabled, AgcModes& mode);

  virtual int SetAgcConfig(const AgcConfig config);

  virtual int GetAgcConfig(AgcConfig& config);

  virtual int SetRxNsStatus(int channel,
                            bool enable,
                            NsModes mode = kNsUnchanged);

  virtual int GetRxNsStatus(int channel, bool& enabled, NsModes& mode);

  virtual int SetRxAgcStatus(int channel,
                             bool enable,
                             AgcModes mode = kAgcUnchanged);

  virtual int GetRxAgcStatus(int channel, bool& enabled, AgcModes& mode);

  virtual int SetRxAgcConfig(int channel, const AgcConfig config);

  virtual int GetRxAgcConfig(int channel, AgcConfig& config);

  virtual int SetEcStatus(bool enable, EcModes mode = kEcUnchanged);

  virtual int GetEcStatus(bool& enabled, EcModes& mode);

  virtual void SetDelayOffsetMs(int offset);

  virtual int DelayOffsetMs();

  virtual int SetAecmMode(AecmModes mode = kAecmSpeakerphone,
                          bool enableCNG = true);

  virtual int GetAecmMode(AecmModes& mode, bool& enabledCNG);

  virtual int RegisterRxVadObserver(int channel,
                                    VoERxVadCallback& observer);

  virtual int DeRegisterRxVadObserver(int channel);

  virtual int VoiceActivityIndicator(int channel);

  virtual int SetEcMetricsStatus(bool enable);

  virtual int GetEcMetricsStatus(bool& enabled);

  virtual int GetEchoMetrics(int& ERL, int& ERLE, int& RERL, int& A_NLP);

  virtual int GetEcDelayMetrics(int& delay_median, int& delay_std);

  virtual int StartDebugRecording(const char* fileNameUTF8);

  virtual int StopDebugRecording();

  virtual int SetTypingDetectionStatus(bool enable);

  virtual int GetTypingDetectionStatus(bool& enabled);

  virtual int TimeSinceLastTyping(int &seconds);

  virtual int SetTypingDetectionParameters(int timeWindow,
                                           int costPerTyping,
                                           int reportingThreshold,
                                           int penaltyDecay);

 protected:
  VoEAudioProcessingImpl(voe::SharedData* shared);
  virtual ~VoEAudioProcessingImpl();

 private:
  bool _isAecMode;
  voe::SharedData* _shared;
};

}  

#endif  

