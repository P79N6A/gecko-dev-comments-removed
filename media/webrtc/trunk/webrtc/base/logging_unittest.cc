









#include "webrtc/base/fileutils.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/pathutils.h"
#include "webrtc/base/stream.h"
#include "webrtc/base/thread.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace rtc {



TEST(LogTest, SingleStream) {
  int sev = LogMessage::GetLogToStream(NULL);

  std::string str;
  StringStream stream(str);
  LogMessage::AddLogToStream(&stream, LS_INFO);
  EXPECT_EQ(LS_INFO, LogMessage::GetLogToStream(&stream));

  LOG(LS_INFO) << "INFO";
  LOG(LS_VERBOSE) << "VERBOSE";
  EXPECT_NE(std::string::npos, str.find("INFO"));
  EXPECT_EQ(std::string::npos, str.find("VERBOSE"));

  LogMessage::RemoveLogToStream(&stream);
  EXPECT_EQ(LogMessage::NO_LOGGING, LogMessage::GetLogToStream(&stream));

  EXPECT_EQ(sev, LogMessage::GetLogToStream(NULL));
}




TEST(LogTest, MultipleStreams) {
  int sev = LogMessage::GetLogToStream(NULL);

  std::string str1, str2;
  StringStream stream1(str1), stream2(str2);
  LogMessage::AddLogToStream(&stream1, LS_INFO);
  LogMessage::AddLogToStream(&stream2, LS_VERBOSE);
  EXPECT_EQ(LS_INFO, LogMessage::GetLogToStream(&stream1));
  EXPECT_EQ(LS_VERBOSE, LogMessage::GetLogToStream(&stream2));

  LOG(LS_INFO) << "INFO";
  LOG(LS_VERBOSE) << "VERBOSE";

  EXPECT_NE(std::string::npos, str1.find("INFO"));
  EXPECT_EQ(std::string::npos, str1.find("VERBOSE"));
  EXPECT_NE(std::string::npos, str2.find("INFO"));
  EXPECT_NE(std::string::npos, str2.find("VERBOSE"));

  LogMessage::RemoveLogToStream(&stream2);
  LogMessage::RemoveLogToStream(&stream1);
  EXPECT_EQ(LogMessage::NO_LOGGING, LogMessage::GetLogToStream(&stream2));
  EXPECT_EQ(LogMessage::NO_LOGGING, LogMessage::GetLogToStream(&stream1));

  EXPECT_EQ(sev, LogMessage::GetLogToStream(NULL));
}



class LogThread : public Thread {
 public:
  virtual ~LogThread() {
    Stop();
  }

 private:
  void Run() {
    
    LOG(LS_SENSITIVE) << "LOG";
  }
};

TEST(LogTest, MultipleThreads) {
  int sev = LogMessage::GetLogToStream(NULL);

  LogThread thread1, thread2, thread3;
  thread1.Start();
  thread2.Start();
  thread3.Start();

  NullStream stream1, stream2, stream3;
  for (int i = 0; i < 1000; ++i) {
    LogMessage::AddLogToStream(&stream1, LS_INFO);
    LogMessage::AddLogToStream(&stream2, LS_VERBOSE);
    LogMessage::AddLogToStream(&stream3, LS_SENSITIVE);
    LogMessage::RemoveLogToStream(&stream1);
    LogMessage::RemoveLogToStream(&stream2);
    LogMessage::RemoveLogToStream(&stream3);
  }

  EXPECT_EQ(sev, LogMessage::GetLogToStream(NULL));
}


TEST(LogTest, WallClockStartTime) {
  uint32 time = LogMessage::WallClockStartTime();
  
  EXPECT_GT(time, 1325376000u);
}


TEST(LogTest, Perf) {
  Pathname path;
  EXPECT_TRUE(Filesystem::GetTemporaryFolder(path, true, NULL));
  path.SetPathname(Filesystem::TempFilename(path, "ut"));

  FileStream stream;
  EXPECT_TRUE(stream.Open(path.pathname(), "wb", NULL));
  stream.DisableBuffering();
  LogMessage::AddLogToStream(&stream, LS_SENSITIVE);

  uint32 start = Time(), finish;
  std::string message('X', 80);
  for (int i = 0; i < 1000; ++i) {
    LOG(LS_SENSITIVE) << message;
  }
  finish = Time();

  LogMessage::RemoveLogToStream(&stream);
  stream.Close();
  Filesystem::DeleteFile(path);

  LOG(LS_INFO) << "Average log time: " << TimeDiff(finish, start) << " us";
}

}  
