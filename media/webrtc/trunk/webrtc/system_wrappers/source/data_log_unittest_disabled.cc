









#include "webrtc/system_wrappers/interface/data_log.h"

#include <cstdio>

#include "gtest/gtest.h"

using ::webrtc::DataLog;

const char* kDataLogFileName = "table_1.txt";

void PerformLogging(std::string table_name) {
  
  ASSERT_EQ(0, DataLog::AddTable(table_name));
  ASSERT_EQ(0, DataLog::AddColumn(table_name, "test", 1));
  for (int i = 0; i < 10; ++i) {
    
    
    DataLog::InsertCell(table_name, "test", static_cast<double>(i));
    ASSERT_EQ(0, DataLog::NextRow(table_name));
  }
}



TEST(TestDataLogDisabled, VerifyLoggingWorks) {
  ASSERT_EQ(0, DataLog::CreateLog());
  
  
  std::string table_name = DataLog::Combine("table", 1);
  ASSERT_EQ("", table_name);
  PerformLogging(table_name);
  DataLog::ReturnLog();
}

TEST(TestDataLogDisabled, EnsureNoFileIsWritten) {
  
  std::remove(kDataLogFileName);
  ASSERT_EQ(0, DataLog::CreateLog());
  
  
  PerformLogging("table_1");
  DataLog::ReturnLog();
  
  ASSERT_EQ(NULL, fopen(kDataLogFileName, "r"));
}
