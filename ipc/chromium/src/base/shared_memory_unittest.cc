



#include "base/basictypes.h"
#include "base/multiprocess_test.h"
#include "base/platform_thread.h"
#include "base/scoped_nsautorelease_pool.h"
#include "base/shared_memory.h"
#include "base/scoped_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

static const int kNumThreads = 5;
static const int kNumTasks = 5;

namespace base {

namespace {




class MultipleThreadMain : public PlatformThread::Delegate {
 public:
  explicit MultipleThreadMain(int16 id) : id_(id) {}
  ~MultipleThreadMain() {}

  static void CleanUp() {
    SharedMemory memory;
    memory.Delete(s_test_name_);
  }

  
  void ThreadMain() {
    ScopedNSAutoreleasePool pool;  
    const int kDataSize = 1024;
    SharedMemory memory;
    bool rv = memory.Create(s_test_name_, false, true, kDataSize);
    EXPECT_TRUE(rv);
    rv = memory.Map(kDataSize);
    EXPECT_TRUE(rv);
    int *ptr = static_cast<int*>(memory.memory()) + id_;
    EXPECT_EQ(*ptr, 0);

    for (int idx = 0; idx < 100; idx++) {
      *ptr = idx;
      PlatformThread::Sleep(1);  
      EXPECT_EQ(*ptr, idx);
    }

    memory.Close();
  }

 private:
  int16 id_;

  static const wchar_t* const s_test_name_;

  DISALLOW_COPY_AND_ASSIGN(MultipleThreadMain);
};

const wchar_t* const MultipleThreadMain::s_test_name_ =
    L"SharedMemoryOpenThreadTest";




#if defined(OS_WIN)




class MultipleLockThread : public PlatformThread::Delegate {
 public:
  explicit MultipleLockThread(int id) : id_(id) {}
  ~MultipleLockThread() {}

  
  void ThreadMain() {
    const int kDataSize = sizeof(int);
    SharedMemoryHandle handle = NULL;
    {
      SharedMemory memory1;
      EXPECT_TRUE(memory1.Create(L"SharedMemoryMultipleLockThreadTest",
                                 false, true, kDataSize));
      EXPECT_TRUE(memory1.ShareToProcess(GetCurrentProcess(), &handle));
      
      
      EXPECT_TRUE(true);
    }

    SharedMemory memory2(handle, false);
    EXPECT_TRUE(memory2.Map(kDataSize));
    volatile int* const ptr = static_cast<int*>(memory2.memory());

    for (int idx = 0; idx < 20; idx++) {
      memory2.Lock();
      int i = (id_ << 16) + idx;
      *ptr = i;
      PlatformThread::Sleep(1);  
      EXPECT_EQ(*ptr, i);
      memory2.Unlock();
    }

    memory2.Close();
  }

 private:
  int id_;

  DISALLOW_COPY_AND_ASSIGN(MultipleLockThread);
};
#endif

}  

TEST(SharedMemoryTest, OpenClose) {
  const int kDataSize = 1024;
  std::wstring test_name = L"SharedMemoryOpenCloseTest";

  
  
  SharedMemory memory1;
  bool rv = memory1.Delete(test_name);
  EXPECT_TRUE(rv);
  rv = memory1.Delete(test_name);
  EXPECT_TRUE(rv);
  rv = memory1.Open(test_name, false);
  EXPECT_FALSE(rv);
  rv = memory1.Create(test_name, false, false, kDataSize);
  EXPECT_TRUE(rv);
  rv = memory1.Map(kDataSize);
  EXPECT_TRUE(rv);
  SharedMemory memory2;
  rv = memory2.Open(test_name, false);
  EXPECT_TRUE(rv);
  rv = memory2.Map(kDataSize);
  EXPECT_TRUE(rv);
  EXPECT_NE(memory1.memory(), memory2.memory());  

  
  ASSERT_NE(memory1.memory(), static_cast<void*>(NULL));
  ASSERT_NE(memory2.memory(), static_cast<void*>(NULL));

  
  memset(memory1.memory(), '1', kDataSize);
  EXPECT_EQ(memcmp(memory1.memory(), memory2.memory(), kDataSize), 0);

  
  memory1.Close();
  char *start_ptr = static_cast<char *>(memory2.memory());
  char *end_ptr = start_ptr + kDataSize;
  for (char* ptr = start_ptr; ptr < end_ptr; ptr++)
    EXPECT_EQ(*ptr, '1');

  
  memory2.Close();

  rv = memory1.Delete(test_name);
  EXPECT_TRUE(rv);
  rv = memory2.Delete(test_name);
  EXPECT_TRUE(rv);
}



TEST(SharedMemoryTest, MultipleThreads) {
  MultipleThreadMain::CleanUp();
  
  
  
  
  
  

  int threadcounts[] = { 1, kNumThreads };
  for (size_t i = 0; i < sizeof(threadcounts) / sizeof(threadcounts); i++) {
    int numthreads = threadcounts[i];
    scoped_array<PlatformThreadHandle> thread_handles;
    scoped_array<MultipleThreadMain*> thread_delegates;

    thread_handles.reset(new PlatformThreadHandle[numthreads]);
    thread_delegates.reset(new MultipleThreadMain*[numthreads]);

    
    for (int16 index = 0; index < numthreads; index++) {
      PlatformThreadHandle pth;
      thread_delegates[index] = new MultipleThreadMain(index);
      EXPECT_TRUE(PlatformThread::Create(0, thread_delegates[index], &pth));
      thread_handles[index] = pth;
    }

    
    for (int index = 0; index < numthreads; index++) {
      PlatformThread::Join(thread_handles[index]);
      delete thread_delegates[index];
    }
  }
  MultipleThreadMain::CleanUp();
}





#if defined(OS_WIN)



TEST(SharedMemoryTest, Lock) {
  PlatformThreadHandle thread_handles[kNumThreads];
  MultipleLockThread* thread_delegates[kNumThreads];

  
  for (int index = 0; index < kNumThreads; ++index) {
    PlatformThreadHandle pth;
    thread_delegates[index] = new MultipleLockThread(index);
    EXPECT_TRUE(PlatformThread::Create(0, thread_delegates[index], &pth));
    thread_handles[index] = pth;
  }

  
  for (int index = 0; index < kNumThreads; ++index) {
    PlatformThread::Join(thread_handles[index]);
    delete thread_delegates[index];
  }
}
#endif




TEST(SharedMemoryTest, AnonymousPrivate) {
  int i, j;
  int count = 4;
  bool rv;
  const int kDataSize = 8192;

  scoped_array<SharedMemory> memories(new SharedMemory[count]);
  scoped_array<int*> pointers(new int*[count]);
  ASSERT_TRUE(memories.get());
  ASSERT_TRUE(pointers.get());

  for (i = 0; i < count; i++) {
    rv = memories[i].Create(L"", false, true, kDataSize);
    EXPECT_TRUE(rv);
    rv = memories[i].Map(kDataSize);
    EXPECT_TRUE(rv);
    int *ptr = static_cast<int*>(memories[i].memory());
    EXPECT_TRUE(ptr);
    pointers[i] = ptr;
  }

  for (i = 0; i < count; i++) {
    
    for (j = 0; j < count; j++) {
      if (i == j)
        pointers[j][0] = 100;
      else
        pointers[j][0] = 0;
    }
    
    for (j = 0; j < count; j++) {
      if (i == j)
        EXPECT_EQ(100, pointers[j][0]);
      else
        EXPECT_EQ(0, pointers[j][0]);
    }
  }

  for (int i = 0; i < count; i++) {
    memories[i].Close();
  }

}




class SharedMemoryProcessTest : public MultiProcessTest {
 public:

  static void CleanUp() {
    SharedMemory memory;
    memory.Delete(s_test_name_);
  }

  static int TaskTestMain() {
    int errors = 0;
    ScopedNSAutoreleasePool pool;  
    const int kDataSize = 1024;
    SharedMemory memory;
    bool rv = memory.Create(s_test_name_, false, true, kDataSize);
    EXPECT_TRUE(rv);
    if (rv != true)
      errors++;
    rv = memory.Map(kDataSize);
    EXPECT_TRUE(rv);
    if (rv != true)
      errors++;
    int *ptr = static_cast<int*>(memory.memory());

    for (int idx = 0; idx < 20; idx++) {
      memory.Lock();
      int i = (1 << 16) + idx;
      *ptr = i;
      PlatformThread::Sleep(10);  
      if (*ptr != i)
        errors++;
      memory.Unlock();
    }

    memory.Close();
    return errors;
  }

 private:
  static const wchar_t* const s_test_name_;
};

const wchar_t* const SharedMemoryProcessTest::s_test_name_ = L"MPMem";


TEST_F(SharedMemoryProcessTest, Tasks) {
  SharedMemoryProcessTest::CleanUp();

  base::ProcessHandle handles[kNumTasks];
  for (int index = 0; index < kNumTasks; ++index) {
    handles[index] = SpawnChild(L"SharedMemoryTestMain");
  }

  int exit_code = 0;
  for (int index = 0; index < kNumTasks; ++index) {
    EXPECT_TRUE(base::WaitForExitCode(handles[index], &exit_code));
    EXPECT_TRUE(exit_code == 0);
  }

  SharedMemoryProcessTest::CleanUp();
}

MULTIPROCESS_TEST_MAIN(SharedMemoryTestMain) {
  return SharedMemoryProcessTest::TaskTestMain();
}


}  
