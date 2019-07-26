









#include "webrtc/test/flags.h"

#include "gflags/gflags.h"

namespace webrtc {
namespace test {
namespace flags {

void Init(int* argc, char*** argv) {
  
  
  google::AllowCommandLineReparsing();
  google::ParseCommandLineFlags(argc, argv, true);
}

DEFINE_int32(width, 640, "Video width.");
size_t Width() { return static_cast<size_t>(FLAGS_width); }

DEFINE_int32(height, 480, "Video height.");
size_t Height() { return static_cast<size_t>(FLAGS_height); }

DEFINE_int32(fps, 30, "Frames per second.");
int Fps() { return static_cast<int>(FLAGS_fps); }

DEFINE_int32(min_bitrate, 50, "Minimum video bitrate.");
size_t MinBitrate() { return static_cast<size_t>(FLAGS_min_bitrate); }

DEFINE_int32(start_bitrate, 300, "Video starting bitrate.");
size_t StartBitrate() { return static_cast<size_t>(FLAGS_start_bitrate); }

DEFINE_int32(max_bitrate, 800, "Maximum video bitrate.");
size_t MaxBitrate() { return static_cast<size_t>(FLAGS_max_bitrate); }
}  
}  
}  
