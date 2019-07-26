









#ifndef WEBRTC_TOOLS_FRAME_ANALYZER_VIDEO_QUALITY_ANALYSIS_H_
#define WEBRTC_TOOLS_FRAME_ANALYZER_VIDEO_QUALITY_ANALYSIS_H_

#include <string>
#include <vector>

#include "third_party/libyuv/include/libyuv/compare.h"
#include "third_party/libyuv/include/libyuv/convert.h"

namespace webrtc {
namespace test {

struct AnalysisResult {
  AnalysisResult() {}
  AnalysisResult(int frame_number, double psnr_value, double ssim_value)
      : frame_number(frame_number),
        psnr_value(psnr_value),
        ssim_value(ssim_value) {}
  int frame_number;
  double psnr_value;
  double ssim_value;
};

struct ResultsContainer {
  std::vector<AnalysisResult> frames;
};

enum VideoAnalysisMetricsType {kPSNR, kSSIM};














void RunAnalysis(const char* reference_file_name, const char* test_file_name,
                 const char* stats_file_name, int width, int height,
                 ResultsContainer* results);





double CalculateMetrics(VideoAnalysisMetricsType video_metrics_type,
                        const uint8* ref_frame,  const uint8* test_frame,
                        int width, int height);




void PrintAnalysisResults(const std::string& label, ResultsContainer* results);


void PrintAnalysisResults(FILE* output, const std::string& label,
                          ResultsContainer* results);



void PrintMaxRepeatedAndSkippedFrames(const std::string& label,
                                      const std::string& stats_file_name);


void PrintMaxRepeatedAndSkippedFrames(FILE* output, const std::string& label,
                                      const std::string& stats_file_name);


bool GetNextStatsLine(FILE* stats_file, char* line);


int GetI420FrameSize(int width, int height);



int ExtractFrameSequenceNumber(std::string line);


bool IsThereBarcodeError(std::string line);



int ExtractDecodedFrameNumber(std::string line);


bool GetNextI420Frame(FILE* input_file, int width, int height,
                      uint8* result_frame);


bool ExtractFrameFromI420(const char* i420_file_name, int width, int height,
                          int frame_number, uint8* result_frame);


}  
}  

#endif  
