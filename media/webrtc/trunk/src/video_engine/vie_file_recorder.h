









#ifndef WEBRTC_VIDEO_ENGINE_VIE_FILE_RECORDER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_FILE_RECORDER_H_

#include "modules/utility/interface/file_recorder.h"
#include "typedefs.h"
#include "video_engine/include/vie_file.h"
#include "voice_engine/main/interface/voe_file.h"

namespace webrtc {

class CriticalSectionWrapper;

class ViEFileRecorder : protected OutStream {
 public:
  explicit ViEFileRecorder(int channel_id);
  ~ViEFileRecorder();

  int StartRecording(const char* file_nameUTF8,
                     const VideoCodec& codec_inst,
                     AudioSource audio_source, int audio_channel,
                     const CodecInst& audio_codec_inst,
                     VoiceEngine* voe_ptr,
                     const FileFormats file_format = kFileFormatAviFile);
  int StopRecording();

  void SetFrameDelay(int frame_delay);
  bool RecordingStarted();

  
  void RecordVideoFrame(const VideoFrame& video_frame);

 protected:
  bool FirstFrameRecorded();
  bool IsRecordingFileFormat(const FileFormats file_format);

  
  bool Write(const void* buf, int len);
  int Rewind();

 private:
  CriticalSectionWrapper* recorder_cs_;

  FileRecorder* file_recorder_;
  bool is_first_frame_recorded_;
  bool is_out_stream_started_;
  int instance_id_;
  int frame_delay_;
  int audio_channel_;
  AudioSource audio_source_;
  VoEFile* voe_file_interface_;
};

}  

#endif  
