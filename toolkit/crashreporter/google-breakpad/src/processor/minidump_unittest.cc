































#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "breakpad_googletest_includes.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/minidump.h"
#include "processor/logging.h"

namespace {

using google_breakpad::Minidump;
using std::ifstream;
using std::istringstream;
using std::string;
using std::vector;
using ::testing::Return;

class MinidumpTest : public ::testing::Test {
public:
  void SetUp() {
    minidump_file_ = string(getenv("srcdir") ? getenv("srcdir") : ".") +
      "/src/processor/testdata/minidump2.dmp";
  }
  string minidump_file_;
};

TEST_F(MinidumpTest, TestMinidumpFromFile) {
  Minidump minidump(minidump_file_);
  ASSERT_EQ(minidump.path(), minidump_file_);
  ASSERT_TRUE(minidump.Read());
  const MDRawHeader* header = minidump.header();
  ASSERT_NE(header, (MDRawHeader*)NULL);
  ASSERT_EQ(header->signature, MD_HEADER_SIGNATURE);
  
}

TEST_F(MinidumpTest, TestMinidumpFromStream) {
  
  ifstream file_stream(minidump_file_.c_str(), std::ios::in);
  ASSERT_TRUE(file_stream.good());
  vector<char> bytes;
  file_stream.seekg(0, std::ios_base::end);
  ASSERT_TRUE(file_stream.good());
  bytes.resize(file_stream.tellg());
  file_stream.seekg(0, std::ios_base::beg);
  ASSERT_TRUE(file_stream.good());
  file_stream.read(&bytes[0], bytes.size());
  ASSERT_TRUE(file_stream.good());
  string str(&bytes[0], bytes.size());
  istringstream stream(str);
  ASSERT_TRUE(stream.good());

  
  Minidump minidump(stream);
  ASSERT_EQ(minidump.path(), "");
  ASSERT_TRUE(minidump.Read());
  const MDRawHeader* header = minidump.header();
  ASSERT_NE(header, (MDRawHeader*)NULL);
  ASSERT_EQ(header->signature, MD_HEADER_SIGNATURE);
  
}

}  

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
