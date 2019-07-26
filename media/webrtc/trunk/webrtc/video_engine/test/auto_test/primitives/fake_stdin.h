








#ifndef FAKE_STDIN_H_
#define FAKE_STDIN_H_

#include <stdio.h>

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace webrtc {


FILE* FakeStdin(const std::string& input);

}  

#endif  
