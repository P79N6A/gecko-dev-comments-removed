









#ifndef WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_SOURCE_FRAMEDROP_PRIMITIVES_H_
#define WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_SOURCE_FRAMEDROP_PRIMITIVES_H_

#include <map>
#include <vector>

#include "video_engine/include/vie_codec.h"
#include "video_engine/include/vie_image_process.h"
#include "video_engine/test/auto_test/interface/vie_autotest_defines.h"
#include "video_engine/test/libvietest/include/vie_to_file_renderer.h"

class FrameDropDetector;
struct NetworkParameters;
class TbInterfaces;






void TestFullStack(const TbInterfaces& interfaces,
                   int capture_id,
                   int video_channel,
                   int width,
                   int height,
                   int bit_rate_kbps,
                   const NetworkParameters& network,
                   FrameDropDetector* frame_drop_detector,
                   ViEToFileRenderer* remote_file_renderer,
                   ViEToFileRenderer* local_file_renderer);




class Frame {
 public:
  Frame(int number, unsigned int timestamp)
    : number_(number),
      frame_timestamp_(timestamp),
      created_timestamp_in_us_(-1),
      sent_timestamp_in_us_(-1),
      received_timestamp_in_us_(-1),
      decoded_timestamp_in_us_(-1),
      rendered_timestamp_in_us_(-1),
      dropped_at_send(false),
      dropped_at_receive(false),
      dropped_at_decode(false),
      dropped_at_render(false) {}

  
  int number_;

  
  
  unsigned int frame_timestamp_;

  
  int64_t created_timestamp_in_us_;
  int64_t sent_timestamp_in_us_;
  int64_t received_timestamp_in_us_;
  int64_t decoded_timestamp_in_us_;
  int64_t rendered_timestamp_in_us_;

  
  bool dropped_at_send;
  bool dropped_at_receive;
  bool dropped_at_decode;
  bool dropped_at_render;
};












void FixOutputFileForComparison(const std::string& output_file,
                                int frame_length_in_bytes,
                                const std::vector<Frame*>& frames);



















class FrameDropDetector {
 public:
  enum State {
    
    
    
    kCreated,
    
    
    kSent,
    
    
    
    kReceived,
    
    
    kDecoded,
    
    
    
    kRendered
  };

  FrameDropDetector()
      : dirty_(true),
        dropped_frames_at_send_(0),
        dropped_frames_at_receive_(0),
        dropped_frames_at_decode_(0),
        dropped_frames_at_render_(0),
        num_created_frames_(0),
        num_sent_frames_(0),
        num_received_frames_(0),
        num_decoded_frames_(0),
        num_rendered_frames_(0),
        timestamp_diff_(0) {}

  
  void ReportFrameState(State state, unsigned int timestamp,
                        int64_t report_time_us);

  
  
  
  
  void CalculateResults();

  
  
  
  int GetNumberOfFramesDroppedAt(State state);

  
  
  
  const std::vector<Frame*>& GetAllFrames();

  
  
  
  
  
  void PrintReport(const std::string& test_label);

  
  
  void PrintDebugDump();
 private:
  
  
  bool dirty_;

  
  std::map<unsigned int, Frame*> created_frames_;

  
  
  std::map<unsigned int, int64_t> sent_frames_;
  std::map<unsigned int, int64_t> received_frames_;
  std::map<unsigned int, int64_t> decoded_frames_;
  std::map<unsigned int, int64_t> rendered_frames_;

  
  std::vector<Frame*> created_frames_vector_;

  
  int dropped_frames_at_send_;
  int dropped_frames_at_receive_;
  int dropped_frames_at_decode_;
  int dropped_frames_at_render_;

  int num_created_frames_;
  int num_sent_frames_;
  int num_received_frames_;
  int num_decoded_frames_;
  int num_rendered_frames_;

  
  
  unsigned int timestamp_diff_;
};



class FrameDropMonitoringRemoteFileRenderer : public ViEToFileRenderer {
 public:
  explicit FrameDropMonitoringRemoteFileRenderer(
      FrameDropDetector* frame_drop_detector)
      : frame_drop_detector_(frame_drop_detector) {}
  virtual ~FrameDropMonitoringRemoteFileRenderer() {}

  
  int FrameSizeChange(unsigned int width, unsigned int height,
                      unsigned int number_of_streams);
  int DeliverFrame(unsigned char* buffer, int buffer_size,
                   uint32_t time_stamp,
                   int64_t render_time);
 private:
  FrameDropDetector* frame_drop_detector_;
};

#endif
