



#include "base/directory_watcher.h"

#include <limits>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/platform_thread.h"
#include "base/string_util.h"
#if defined(OS_WIN)
#include "base/win_util.h"
#endif  
#include "testing/gtest/include/gtest/gtest.h"

namespace {


const int kWaitForEventTime = 1000;

class DirectoryWatcherTest : public testing::Test {
 public:
  
  DirectoryWatcherTest() : loop_(MessageLoop::TYPE_UI) {
  }

  void OnTestDelegateFirstNotification(const FilePath& path) {
    notified_delegates_++;
    if (notified_delegates_ >= expected_notified_delegates_)
      MessageLoop::current()->Quit();
  }

 protected:
  virtual void SetUp() {
    
    FilePath path;
    ASSERT_TRUE(PathService::Get(base::DIR_TEMP, &path));
    test_dir_ = path.Append(FILE_PATH_LITERAL("DirectoryWatcherTest"));

    
    file_util::Delete(test_dir_, true);
    file_util::CreateDirectory(test_dir_);
  }

  virtual void TearDown() {
    
    loop_.RunAllPending();

    
    ASSERT_TRUE(file_util::Delete(test_dir_, true));
    ASSERT_FALSE(file_util::PathExists(test_dir_));
  }

  
  bool WriteTestFile(const FilePath& filename,
                     const std::string& content) {
    return (file_util::WriteFile(filename, content.c_str(), content.length()) ==
            static_cast<int>(content.length()));
  }

  
  
  FilePath CreateTestDirDirectoryASCII(const std::string& name, bool sync) {
    FilePath path(test_dir_.AppendASCII(name));
    EXPECT_TRUE(file_util::CreateDirectory(path));
    if (sync)
      SyncIfPOSIX();
    return path;
  }

  void SetExpectedNumberOfNotifiedDelegates(int n) {
    notified_delegates_ = 0;
    expected_notified_delegates_ = n;
  }

  void VerifyExpectedNumberOfNotifiedDelegates() {
    
    if (expected_notified_delegates_ - notified_delegates_ > 0)
      loop_.Run();

    
    loop_.PostDelayedTask(FROM_HERE, new MessageLoop::QuitTask,
                          kWaitForEventTime);
    loop_.Run();
    EXPECT_EQ(expected_notified_delegates_, notified_delegates_);
  }

  
  
  
  
  void SyncIfPOSIX() {
#if defined(OS_POSIX)
    sync();
#endif  
  }

  MessageLoop loop_;

  
  FilePath test_dir_;

  
  int notified_delegates_;

  
  int expected_notified_delegates_;
};

class TestDelegate : public DirectoryWatcher::Delegate {
 public:
  TestDelegate(DirectoryWatcherTest* test)
      : test_(test),
        got_notification_(false),
        original_thread_id_(PlatformThread::CurrentId()) {
  }

  bool got_notification() const {
    return got_notification_;
  }

  void reset() {
    got_notification_ = false;
  }

  virtual void OnDirectoryChanged(const FilePath& path) {
    EXPECT_EQ(original_thread_id_, PlatformThread::CurrentId());
    if (!got_notification_)
      test_->OnTestDelegateFirstNotification(path);
    got_notification_ = true;
  }

 private:
  
  DirectoryWatcherTest* test_;

  
  bool got_notification_;

  
  
  PlatformThreadId original_thread_id_;
};


TEST_F(DirectoryWatcherTest, NewFile) {
  DirectoryWatcher watcher;
  TestDelegate delegate(this);
  ASSERT_TRUE(watcher.Watch(test_dir_, &delegate, false));

  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(WriteTestFile(test_dir_.AppendASCII("test_file"), "content"));
  VerifyExpectedNumberOfNotifiedDelegates();
}


TEST_F(DirectoryWatcherTest, ModifiedFile) {
  
  ASSERT_TRUE(WriteTestFile(test_dir_.AppendASCII("test_file"), "content"));
  SyncIfPOSIX();

  DirectoryWatcher watcher;
  TestDelegate delegate(this);
  ASSERT_TRUE(watcher.Watch(test_dir_, &delegate, false));

  
  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(WriteTestFile(test_dir_.AppendASCII("test_file"), "new content"));
  VerifyExpectedNumberOfNotifiedDelegates();
}

TEST_F(DirectoryWatcherTest, DeletedFile) {
  
  ASSERT_TRUE(WriteTestFile(test_dir_.AppendASCII("test_file"), "content"));
  SyncIfPOSIX();

  DirectoryWatcher watcher;
  TestDelegate delegate(this);
  ASSERT_TRUE(watcher.Watch(test_dir_, &delegate, false));

  
  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(file_util::Delete(test_dir_.AppendASCII("test_file"), false));
  VerifyExpectedNumberOfNotifiedDelegates();
}


TEST_F(DirectoryWatcherTest, Unregister) {
  TestDelegate delegate(this);

  {
    DirectoryWatcher watcher;
    ASSERT_TRUE(watcher.Watch(test_dir_, &delegate, false));

    
  }

  
  SetExpectedNumberOfNotifiedDelegates(0);
  ASSERT_TRUE(WriteTestFile(test_dir_.AppendASCII("test_file"), "content"));
  VerifyExpectedNumberOfNotifiedDelegates();
}

TEST_F(DirectoryWatcherTest, SubDirRecursive) {
  FilePath subdir(CreateTestDirDirectoryASCII("SubDir", true));

#if defined(OS_LINUX)
  
  return;
#endif  

  
  TestDelegate delegate(this);
  DirectoryWatcher watcher;
  ASSERT_TRUE(watcher.Watch(test_dir_, &delegate, true));
  
  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(WriteTestFile(subdir.AppendASCII("test_file"), "some content"));
  VerifyExpectedNumberOfNotifiedDelegates();
}

TEST_F(DirectoryWatcherTest, SubDirNonRecursive) {
#if defined(OS_WIN)
  
  
  if (win_util::GetWinVersion() < win_util::WINVERSION_VISTA)
    return;
#endif  

  FilePath subdir(CreateTestDirDirectoryASCII("SubDir", false));

  
  
  ASSERT_TRUE(WriteTestFile(subdir.AppendASCII("test_file"), "some content"));

  SyncIfPOSIX();

  
  
  DirectoryWatcher watcher;
  TestDelegate delegate(this);
  ASSERT_TRUE(watcher.Watch(test_dir_, &delegate, false));

  
  SetExpectedNumberOfNotifiedDelegates(0);
  ASSERT_TRUE(WriteTestFile(subdir.AppendASCII("test_file"), "other content"));
  VerifyExpectedNumberOfNotifiedDelegates();
}

namespace {


class Deleter : public DirectoryWatcher::Delegate {
 public:
  Deleter(DirectoryWatcher* watcher, MessageLoop* loop)
      : watcher_(watcher),
        loop_(loop) {
  }

  virtual void OnDirectoryChanged(const FilePath& path) {
    watcher_.reset(NULL);
    loop_->PostTask(FROM_HERE, new MessageLoop::QuitTask());
  }

  scoped_ptr<DirectoryWatcher> watcher_;
  MessageLoop* loop_;
};
}  


TEST_F(DirectoryWatcherTest, DeleteDuringNotify) {
  DirectoryWatcher* watcher = new DirectoryWatcher;
  Deleter deleter(watcher, &loop_);  
  ASSERT_TRUE(watcher->Watch(test_dir_, &deleter, false));

  ASSERT_TRUE(WriteTestFile(test_dir_.AppendASCII("test_file"), "content"));
  loop_.Run();

  
  
  ASSERT_TRUE(deleter.watcher_.get() == NULL);
}

TEST_F(DirectoryWatcherTest, MultipleWatchersSingleFile) {
  DirectoryWatcher watcher1, watcher2;
  TestDelegate delegate1(this), delegate2(this);
  ASSERT_TRUE(watcher1.Watch(test_dir_, &delegate1, false));
  ASSERT_TRUE(watcher2.Watch(test_dir_, &delegate2, false));

  SetExpectedNumberOfNotifiedDelegates(2);
  ASSERT_TRUE(WriteTestFile(test_dir_.AppendASCII("test_file"), "content"));
  VerifyExpectedNumberOfNotifiedDelegates();
}

TEST_F(DirectoryWatcherTest, MultipleWatchersDifferentFiles) {
  const int kNumberOfWatchers = 5;
  DirectoryWatcher watchers[kNumberOfWatchers];
  TestDelegate delegates[kNumberOfWatchers] = {this, this, this, this, this};
  FilePath subdirs[kNumberOfWatchers];
  for (int i = 0; i < kNumberOfWatchers; i++) {
    subdirs[i] = CreateTestDirDirectoryASCII("Dir" + IntToString(i), false);
    ASSERT_TRUE(watchers[i].Watch(subdirs[i], &delegates[i], false));
  }
  for (int i = 0; i < kNumberOfWatchers; i++) {
    
    

    for (int j = 0; j < kNumberOfWatchers; j++)
      delegates[j].reset();

    
    SetExpectedNumberOfNotifiedDelegates(1);
    ASSERT_TRUE(WriteTestFile(subdirs[i].AppendASCII("test_file"), "content"));
    VerifyExpectedNumberOfNotifiedDelegates();

    loop_.RunAllPending();
  }
}

#if defined(OS_WIN) || defined(OS_MACOSX)


TEST_F(DirectoryWatcherTest, WatchCreatedDirectory) {
  TestDelegate delegate(this);
  DirectoryWatcher watcher;
  ASSERT_TRUE(watcher.Watch(test_dir_, &delegate, true));

  SetExpectedNumberOfNotifiedDelegates(1);
  FilePath subdir(CreateTestDirDirectoryASCII("SubDir", true));
  
  ASSERT_TRUE(WriteTestFile(subdir.AppendASCII("test_file"), "some content"));
  VerifyExpectedNumberOfNotifiedDelegates();

  delegate.reset();

  
  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(WriteTestFile(subdir.AppendASCII("test_file"), "other content"));
  VerifyExpectedNumberOfNotifiedDelegates();
}

TEST_F(DirectoryWatcherTest, RecursiveWatchDeletedSubdirectory) {
  FilePath subdir(CreateTestDirDirectoryASCII("SubDir", true));

  TestDelegate delegate(this);
  DirectoryWatcher watcher;
  ASSERT_TRUE(watcher.Watch(test_dir_, &delegate, true));

  
  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(WriteTestFile(subdir.AppendASCII("test_file"), "some content"));
  VerifyExpectedNumberOfNotifiedDelegates();

  delegate.reset();

  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(file_util::Delete(subdir, true));
  VerifyExpectedNumberOfNotifiedDelegates();
}

TEST_F(DirectoryWatcherTest, MoveFileAcrossWatches) {
  FilePath subdir1(CreateTestDirDirectoryASCII("SubDir1", true));
  FilePath subdir2(CreateTestDirDirectoryASCII("SubDir2", true));

  TestDelegate delegate1(this), delegate2(this);
  DirectoryWatcher watcher1, watcher2;
  ASSERT_TRUE(watcher1.Watch(subdir1, &delegate1, true));
  ASSERT_TRUE(watcher2.Watch(subdir2, &delegate2, true));

  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(WriteTestFile(subdir1.AppendASCII("file"), "some content"));
  SyncIfPOSIX();
  VerifyExpectedNumberOfNotifiedDelegates();

  delegate1.reset();
  delegate2.reset();

  SetExpectedNumberOfNotifiedDelegates(2);
  ASSERT_TRUE(file_util::Move(subdir1.AppendASCII("file"),
                              subdir2.AppendASCII("file")));
  VerifyExpectedNumberOfNotifiedDelegates();

  delegate1.reset();
  delegate2.reset();

  SetExpectedNumberOfNotifiedDelegates(1);
  ASSERT_TRUE(WriteTestFile(subdir2.AppendASCII("file"), "other content"));
  VerifyExpectedNumberOfNotifiedDelegates();
}
#endif  




TEST_F(DirectoryWatcherTest, NonExistentDirectory) {
  DirectoryWatcher watcher;
  ASSERT_FALSE(watcher.Watch(test_dir_.AppendASCII("does-not-exist"), NULL,
                             false));
}

}  
