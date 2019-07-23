



#include <vector>

#include "base/file_util.h"
#include "base/path_service.h"
#include "base/perftimer.h"
#include "base/string_util.h"
#include "base/values.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/json_value_serializer.h"
#include "chrome/common/logging_chrome.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class JSONValueSerializerTests : public testing::Test {
 protected:
  virtual void SetUp() {
    static const wchar_t* const kTestFilenames[] = {
      L"serializer_nested_test.js",
      L"serializer_test.js",
      L"serializer_test_nowhitespace.js",
    };

    
    for (size_t i = 0; i < arraysize(kTestFilenames); ++i) {
      std::wstring filename;
      EXPECT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &filename));
      file_util::AppendToPath(&filename, kTestFilenames[i]);

      std::string test_case;
      EXPECT_TRUE(file_util::ReadFileToString(filename, &test_case));
      test_cases_.push_back(test_case);
    }
  }

  
  std::vector<std::string> test_cases_;
};

}  



TEST_F(JSONValueSerializerTests, Reading) {
  printf("\n");
  const int kIterations = 100000;

  
  PerfTimeLogger chrome_timer("chrome");
  for (int i = 0; i < kIterations; ++i) {
    for (size_t j = 0; j < test_cases_.size(); ++j) {
      JSONStringValueSerializer reader(test_cases_[j]);
      scoped_ptr<Value> root(reader.Deserialize(NULL));
      ASSERT_TRUE(root.get());
    }
  }
  chrome_timer.Done();
}

TEST_F(JSONValueSerializerTests, CompactWriting) {
  printf("\n");
  const int kIterations = 100000;
  
  std::vector<Value*> test_cases;
  for (size_t i = 0; i < test_cases_.size(); ++i) {
    JSONStringValueSerializer reader(test_cases_[i]);
    Value* root = reader.Deserialize(NULL);
    ASSERT_TRUE(root);
    test_cases.push_back(root);
  }

  PerfTimeLogger chrome_timer("chrome");
  for (int i = 0; i < kIterations; ++i) {
    for (size_t j = 0; j < test_cases.size(); ++j) {
      std::string json;
      JSONStringValueSerializer reader(&json);
      ASSERT_TRUE(reader.Serialize(*test_cases[j]));
    }
  }
  chrome_timer.Done();

  
  for (size_t i = 0; i < test_cases.size(); ++i) {
    delete test_cases[i];
    test_cases[i] = NULL;
  }
}
