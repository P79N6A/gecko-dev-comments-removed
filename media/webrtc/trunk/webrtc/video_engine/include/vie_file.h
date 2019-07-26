














#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_FILE_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_FILE_H_

#include "common_types.h"

namespace webrtc {

class VideoEngine;
struct VideoCodec;


struct ViEPicture {
  unsigned char* data;
  unsigned int size;
  unsigned int width;
  unsigned int height;
  RawVideoType type;

  ViEPicture() {
    data = NULL;
    size = 0;
    width = 0;
    height = 0;
    type = kVideoI420;
  }

  
  ~ViEPicture() {
    data = NULL;
    size = 0;
    width = 0;
    height = 0;
    type = kVideoUnknown;
  }
};


enum AudioSource {
  NO_AUDIO,
  MICROPHONE,
  PLAYOUT,
  VOICECALL
};





class WEBRTC_DLLEXPORT ViEFileObserver {
 public:
  
  virtual void PlayFileEnded(const WebRtc_Word32 file_id) = 0;

 protected:
  virtual ~ViEFileObserver() {}
};

class WEBRTC_DLLEXPORT ViEFile {
 public:
  
  
  
  static ViEFile* GetInterface(VideoEngine* video_engine);

  
  
  
  virtual int Release() = 0;

  
  virtual int StartPlayFile(
      const char* file_name_utf8,
      int& file_id,
      const bool loop = false,
      const FileFormats file_format = kFileFormatAviFile) = 0;

  
  virtual int StopPlayFile(const int file_id) = 0;

  
  virtual int RegisterObserver(int file_id, ViEFileObserver& observer) = 0;

  
  virtual int DeregisterObserver(int file_id, ViEFileObserver& observer) = 0;

  
  virtual int SendFileOnChannel(const int file_id, const int video_channel) = 0;

  
  virtual int StopSendFileOnChannel(const int video_channel) = 0;

  
  
  virtual int StartPlayFileAsMicrophone(const int file_id,
                                        const int audio_channel,
                                        bool mix_microphone = false,
                                        float volume_scaling = 1) = 0;

  
  virtual int StopPlayFileAsMicrophone(const int file_id,
                                       const int audio_channel) = 0;

  
  
  virtual int StartPlayAudioLocally(const int file_id, const int audio_channel,
                                    float volume_scaling = 1) = 0;

  
  virtual int StopPlayAudioLocally(const int file_id,
                                   const int audio_channel) = 0;

  
  virtual int StartRecordOutgoingVideo(
      const int video_channel,
      const char* file_name_utf8,
      AudioSource audio_source,
      const CodecInst& audio_codec,
      const VideoCodec& video_codec,
      const FileFormats file_format = kFileFormatAviFile) = 0;

  
  virtual int StartRecordIncomingVideo(
      const int video_channel,
      const char* file_name_utf8,
      AudioSource audio_source,
      const CodecInst& audio_codec,
      const VideoCodec& video_codec,
      const FileFormats file_format = kFileFormatAviFile) = 0;

  
  virtual int StopRecordOutgoingVideo(const int video_channel) = 0;

  
  virtual int StopRecordIncomingVideo(const int video_channel) = 0;

  
  virtual int GetFileInformation(
      const char* file_name,
      VideoCodec& video_codec,
      CodecInst& audio_codec,
      const FileFormats file_format = kFileFormatAviFile) = 0;

  
  
  virtual int GetRenderSnapshot(const int video_channel,
                                const char* file_name_utf8) = 0;

  
  
  virtual int GetRenderSnapshot(const int video_channel,
                                ViEPicture& picture) = 0;

  
  
  virtual int GetCaptureDeviceSnapshot(const int capture_id,
                                       const char* file_name_utf8) = 0;

  
  
  virtual int GetCaptureDeviceSnapshot(const int capture_id,
                                       ViEPicture& picture) = 0;

  virtual int FreePicture(ViEPicture& picture) = 0;

  
  
  virtual int SetRenderStartImage(const int video_channel,
                                  const char* file_name_utf8) = 0;

  
  
  virtual int SetRenderStartImage(const int video_channel,
                                  const ViEPicture& picture) = 0;

  
  
  virtual int SetRenderTimeoutImage(const int video_channel,
                                    const char* file_name_utf8,
                                    const unsigned int timeout_ms = 1000) = 0;

  
  
  virtual int SetRenderTimeoutImage(const int video_channel,
                                    const ViEPicture& picture,
                                    const unsigned int timeout_ms) = 0;

  
  virtual int StartDebugRecording(int video_channel,
                                  const char* file_name_utf8) = 0;
  
  virtual int StopDebugRecording(int video_channel) = 0;


 protected:
  ViEFile() {}
  virtual ~ViEFile() {}
};

}  

#endif  
