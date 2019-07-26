








#ifndef FAKE_STDIN_H_
#define FAKE_STDIN_H_

#include <cstdio>
#include <string>

#include "gtest/gtest.h"

namespace webrtc {


FILE* FakeStdin(const std::string& input);

}  

#endif  
