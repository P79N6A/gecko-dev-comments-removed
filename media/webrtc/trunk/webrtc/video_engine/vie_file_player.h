









#ifndef WEBRTC_VIDEO_ENGINE_VIE_FILE_PLAYER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_FILE_PLAYER_H_

#include <list>
#include <set>

#include "common_types.h"  
#include "common_video/interface/i420_video_frame.h"
#include "modules/media_file/interface/media_file_defines.h"
#include "system_wrappers/interface/file_wrapper.h"
#include "typedefs.h"  
#include "video_engine/vie_frame_provider_base.h"

namespace webrtc {

class EventWrapper;
class FilePlayer;
class ThreadWrapper;
class ViEFileObserver;
class VoEFile;
class VoEVideoSync;
class VoiceEngine;

class ViEFilePlayer
    : public ViEFrameProviderBase,
      protected FileCallback,
      protected InStream {
 public:
  static ViEFilePlayer* CreateViEFilePlayer(int file_id,
                                            int engine_id,
                                            const char* file_nameUTF8,
                                            const bool loop,
                                            const FileFormats file_format,
                                            VoiceEngine* voe_ptr);

  static int GetFileInformation(const int engine_id,
                                const char* file_name,
                                VideoCodec& video_codec,
                                CodecInst& audio_codec,
                                const FileFormats file_format);
  ~ViEFilePlayer();

  bool IsObserverRegistered();
  int RegisterObserver(ViEFileObserver* observer);
  int DeRegisterObserver();
  int SendAudioOnChannel(const int audio_channel,
                         bool mix_microphone,
                         float volume_scaling);
  int StopSendAudioOnChannel(const int audio_channel);
  int PlayAudioLocally(const int audio_channel, float volume_scaling);
  int StopPlayAudioLocally(const int audio_channel);

  
  virtual int FrameCallbackChanged();

 protected:
  ViEFilePlayer(int Id, int engine_id);
  int Init(const char* file_nameUTF8,
           const bool loop,
           const FileFormats file_format,
           VoiceEngine* voe_ptr);
  int StopPlay();
  int StopPlayAudio();

  
  static bool FilePlayDecodeThreadFunction(void* obj);
  bool FilePlayDecodeProcess();
  bool NeedsAudioFromFile(void* buf);

  
  virtual int Read(void* buf, int len);
  virtual int Rewind() {
    return 0;
  }

  
  virtual void PlayNotification(const WebRtc_Word32 ,
                                const WebRtc_UWord32 ) {}
  virtual void RecordNotification(const WebRtc_Word32 ,
                                  const WebRtc_UWord32 ) {}
  virtual void PlayFileEnded(const WebRtc_Word32 id);
  virtual void RecordFileEnded(const WebRtc_Word32 ) {}

 private:
  static const int kMaxDecodedAudioLength = 320;
  bool play_back_started_;

  CriticalSectionWrapper* feedback_cs_;
  CriticalSectionWrapper* audio_cs_;

  FilePlayer* file_player_;
  bool audio_stream_;

  
  int video_clients_;

  
  int audio_clients_;

  
  int local_audio_channel_;

  ViEFileObserver* observer_;
  char file_name_[FileWrapper::kMaxFileNameSize];

  
  VoEFile* voe_file_interface_;
  VoEVideoSync* voe_video_sync_;

  
  ThreadWrapper* decode_thread_;
  EventWrapper* decode_event_;
  WebRtc_Word16 decoded_audio_[kMaxDecodedAudioLength];
  int decoded_audio_length_;

  
  
  std::list<void*> audio_channel_buffers_;

  
  std::set<int> audio_channels_sending_;

  
  I420VideoFrame decoded_video_;
};

}  

#endif  
