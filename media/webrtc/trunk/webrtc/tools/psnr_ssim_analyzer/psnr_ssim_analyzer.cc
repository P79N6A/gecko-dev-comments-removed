









#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <string>
#include <vector>

#include "webrtc/tools/frame_analyzer/video_quality_analysis.h"
#include "webrtc/tools/simple_command_line_parser.h"

#define MAX_NUM_FRAMES_PER_FILE INT_MAX

void CompareFiles(const char* reference_file_name, const char* test_file_name,
                  const char* results_file_name, int width, int height) {
  
  bool y4m_mode = false;
  if (std::string(reference_file_name).find("y4m") != std::string::npos){
    y4m_mode = true;
  }

  FILE* results_file = fopen(results_file_name, "w");

  int size = webrtc::test::GetI420FrameSize(width, height);

  
  uint8* test_frame = new uint8[size];
  uint8* ref_frame = new uint8[size];

  bool read_result = true;
  for(int frame_counter = 0; frame_counter < MAX_NUM_FRAMES_PER_FILE;
      ++frame_counter){
    read_result &= (y4m_mode) ? webrtc::test::ExtractFrameFromY4mFile(
        reference_file_name, width, height, frame_counter, ref_frame):
        webrtc::test::ExtractFrameFromYuvFile(reference_file_name, width,
                                              height, frame_counter, ref_frame);
    read_result &=  webrtc::test::ExtractFrameFromYuvFile(test_file_name, width,
        height, frame_counter, test_frame);

    if (!read_result)
      break;

    
    double result_psnr = webrtc::test::CalculateMetrics(
        webrtc::test::kPSNR, ref_frame, test_frame, width, height);
    double result_ssim = webrtc::test::CalculateMetrics(
        webrtc::test::kSSIM, ref_frame, test_frame, width, height);
    fprintf(results_file, "Frame: %d, PSNR: %f, SSIM: %f\n", frame_counter,
            result_psnr, result_ssim);
  }
  delete[] test_frame;
  delete[] ref_frame;

  fclose(results_file);
}

















int main(int argc, char** argv) {
  std::string program_name = argv[0];
  std::string usage = "Runs PSNR and SSIM on two I420 videos and write the"
      "results in a file.\n"
      "Example usage:\n" + program_name + " --reference_file=ref.yuv "
      "--test_file=test.yuv --results_file=results.txt --width=320 "
      "--height=240\n"
      "Command line flags:\n"
      "  - width(int): The width of the reference and test files. Default: -1\n"
      "  - height(int): The height of the reference and test files. "
      " Default: -1\n"
      "  - reference_file(string): The reference YUV file to compare against."
      " Default: ref.yuv\n"
      "  - test_file(string): The test YUV file to run the analysis for."
      " Default: test_file.yuv\n"
      "  - results_file(string): The full name of the file where the results "
      "will be written. Default: results.txt\n";

  webrtc::test::CommandLineParser parser;

  
  parser.Init(argc, argv);
  parser.SetUsageMessage(usage);

  parser.SetFlag("width", "-1");
  parser.SetFlag("height", "-1");
  parser.SetFlag("results_file", "results.txt");
  parser.SetFlag("reference_file", "ref.yuv");
  parser.SetFlag("test_file", "test.yuv");
  parser.SetFlag("results_file", "results.txt");
  parser.SetFlag("help", "false");

  parser.ProcessFlags();
  if (parser.GetFlag("help") == "true") {
    parser.PrintUsageMessage();
  }
  parser.PrintEnteredFlags();

  int width = strtol((parser.GetFlag("width")).c_str(), NULL, 10);
  int height = strtol((parser.GetFlag("height")).c_str(), NULL, 10);

  if (width <= 0 || height <= 0) {
    fprintf(stderr, "Error: width or height cannot be <= 0!\n");
    return -1;
  }

  CompareFiles(parser.GetFlag("reference_file").c_str(),
               parser.GetFlag("test_file").c_str(),
               parser.GetFlag("results_file").c_str(), width, height);
}
