









#ifndef SRC_VIDEO_ENGINE_TEST_AUTO_TEST_INTERFACE_VIE_COMPARISON_TESTS_H_
#define SRC_VIDEO_ENGINE_TEST_AUTO_TEST_INTERFACE_VIE_COMPARISON_TESTS_H_

#include <string>

class FrameDropDetector;
class ViEToFileRenderer;















class ViEFileBasedComparisonTests {
 public:
  
  
  bool TestCallSetup(
      const std::string& i420_test_video_path,
      int width,
      int height,
      ViEToFileRenderer* local_file_renderer,
      ViEToFileRenderer* remote_file_renderer);

  
  
  
  bool TestCodecs(
      const std::string& i420_video_file,
      int width,
      int height,
      ViEToFileRenderer* local_file_renderer,
      ViEToFileRenderer* remote_file_renderer);

  
  
  void TestFullStack(
      const std::string& i420_video_file,
      int width,
      int height,
      int bit_rate_kbps,
      int packet_loss_percent,
      int network_delay_ms,
      ViEToFileRenderer* local_file_renderer,
      ViEToFileRenderer* remote_file_renderer,
      FrameDropDetector* frame_drop_detector);
};

#endif  
