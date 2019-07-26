









#include "video_engine/test/auto_test/primitives/fake_stdin.h"

namespace webrtc {

FILE* FakeStdin(const std::string& input) {
  FILE* fake_stdin = tmpfile();

  EXPECT_EQ(input.size(),
      fwrite(input.c_str(), sizeof(char), input.size(), fake_stdin));
  rewind(fake_stdin);

  return fake_stdin;
}

}  
