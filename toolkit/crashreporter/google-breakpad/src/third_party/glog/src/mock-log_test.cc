
































#include "mock-log.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using GOOGLE_NAMESPACE::INFO;
using GOOGLE_NAMESPACE::WARNING;
using GOOGLE_NAMESPACE::ERROR;
using GOOGLE_NAMESPACE::glog_testing::ScopedMockLog;
using std::string;
using testing::_;
using testing::HasSubstr;
using testing::InSequence;
using testing::InvokeWithoutArgs;


TEST(ScopedMockLogTest, InterceptsLog) {
  ScopedMockLog log;

  InSequence s;
  EXPECT_CALL(log, Log(WARNING, HasSubstr("/mock-log_test.cc"), "Fishy."));
  EXPECT_CALL(log, Log(INFO, _, "Working..."))
      .Times(2);
  EXPECT_CALL(log, Log(ERROR, _, "Bad!!"));

  LOG(WARNING) << "Fishy.";
  LOG(INFO) << "Working...";
  LOG(INFO) << "Working...";
  LOG(ERROR) << "Bad!!";
}

void LogBranch() {
  LOG(INFO) << "Logging a branch...";
}

void LogTree() {
  LOG(INFO) << "Logging the whole tree...";
}

void LogForest() {
  LOG(INFO) << "Logging the entire forest.";
  LOG(INFO) << "Logging the entire forest..";
  LOG(INFO) << "Logging the entire forest...";
}




TEST(ScopedMockLogTest, LogDuringIntercept) {
  ScopedMockLog log;
  InSequence s;
  EXPECT_CALL(log, Log(INFO, __FILE__, "Logging a branch..."))
      .WillOnce(InvokeWithoutArgs(LogTree));
  EXPECT_CALL(log, Log(INFO, __FILE__, "Logging the whole tree..."))
      .WillOnce(InvokeWithoutArgs(LogForest));
  EXPECT_CALL(log, Log(INFO, __FILE__, "Logging the entire forest."));
  EXPECT_CALL(log, Log(INFO, __FILE__, "Logging the entire forest.."));
  EXPECT_CALL(log, Log(INFO, __FILE__, "Logging the entire forest..."));
  LogBranch();
}

}  

int main(int argc, char **argv) {
  GOOGLE_NAMESPACE::InitGoogleLogging(argv[0]);
  testing::InitGoogleMock(&argc, argv);

  return RUN_ALL_TESTS();
}
