









#ifndef WEBRTC_TESTSUPPORT_METRICS_VIDEO_METRICS_H_
#define WEBRTC_TESTSUPPORT_METRICS_VIDEO_METRICS_H_

#include <limits>
#include <vector>

namespace webrtc {
namespace test {


extern double kMetricsPerfectPSNR;


struct FrameResult {
  int frame_number;
  double value;
};



struct QualityMetricsResult {
  QualityMetricsResult() :
    average(0.0),
    min(std::numeric_limits<double>::max()),
    max(std::numeric_limits<double>::min()),
    min_frame_number(-1),
    max_frame_number(-1)
  {};
  double average;
  double min;
  double max;
  int min_frame_number;
  int max_frame_number;
  std::vector<FrameResult> frames;
};




















int I420MetricsFromFiles(const char* ref_filename,
                         const char* test_filename,
                         int width,
                         int height,
                         QualityMetricsResult* psnr_result,
                         QualityMetricsResult* ssim_result);



















int I420PSNRFromFiles(const char* ref_filename,
                      const char* test_filename,
                      int width,
                      int height,
                      QualityMetricsResult* result);
















int I420SSIMFromFiles(const char* ref_filename,
                      const char* test_filename,
                      int width,
                      int height,
                      QualityMetricsResult* result);

}  
}  

#endif 
