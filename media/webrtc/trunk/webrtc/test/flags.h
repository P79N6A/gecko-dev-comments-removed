









#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_FLAGS_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_FLAGS_H_

#include <stddef.h>

namespace webrtc {
namespace test {
namespace flags {

void Init(int* argc, char ***argv);

size_t Width();
size_t Height();
int Fps();
size_t MinBitrate();
size_t StartBitrate();
size_t MaxBitrate();
}  
}  
}  

#endif  
