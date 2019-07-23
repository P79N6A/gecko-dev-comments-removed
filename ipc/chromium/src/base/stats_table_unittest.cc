



#include "base/multiprocess_test.h"
#include "base/platform_thread.h"
#include "base/simple_thread.h"
#include "base/shared_memory.h"
#include "base/stats_table.h"
#include "base/stats_counters.h"
#include "base/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

#if defined(OS_WIN)
#include <process.h>
#include <windows.h>
#endif

namespace base {

class StatsTableTest : public MultiProcessTest {
 public:
  void DeleteShmem(std::string name) {
    base::SharedMemory mem;
    mem.Delete(UTF8ToWide(name));
  }
};



TEST_F(StatsTableTest, VerifySlots) {
  const std::string kTableName = "VerifySlotsStatTable";
  const int kMaxThreads = 1;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);

  
  std::string thread_name = "mainThread";
  int slot_id = table.RegisterThread(thread_name);
  EXPECT_TRUE(slot_id);

  
  std::string counter_base_name = "counter";
  for (int index=0; index < kMaxCounter; index++) {
    std::string counter_name = counter_base_name;
    StringAppendF(&counter_name, "counter.ctr%d", index);
    int counter_id = table.FindCounter(counter_name);
    EXPECT_GT(counter_id, 0);
  }

  
  slot_id = table.RegisterThread("too many threads");
  EXPECT_EQ(slot_id, 0);

  
  int counter_id = table.FindCounter(counter_base_name);
  EXPECT_EQ(counter_id, 0);

  DeleteShmem(kTableName);
}


const std::string kCounterZero = "CounterZero";

const std::string kCounter1313 = "Counter1313";

const std::string kCounterIncrement = "CounterIncrement";

const std::string kCounterDecrement = "CounterDecrement";


const std::string kCounterMixed = "CounterMixed";

const int kThreadLoops = 1000;

class StatsTableThread : public base::SimpleThread {
public:
  StatsTableThread(std::string name, int id)
      : base::SimpleThread(name), id_(id) { }
  virtual void Run();
private:
  int id_;
};

void StatsTableThread::Run() {
  
  
  

  StatsCounter zero_counter(kCounterZero);
  StatsCounter lucky13_counter(kCounter1313);
  StatsCounter increment_counter(kCounterIncrement);
  StatsCounter decrement_counter(kCounterDecrement);
  for (int index = 0; index < kThreadLoops; index++) {
    StatsCounter mixed_counter(kCounterMixed);  
    zero_counter.Set(0);
    lucky13_counter.Set(1313);
    increment_counter.Increment();
    decrement_counter.Decrement();
    if (id_ % 2)
      mixed_counter.Decrement();
    else
      mixed_counter.Increment();
    PlatformThread::Sleep(index % 10);   
  }
}




TEST_F(StatsTableTest, MultipleThreads) {
#if 0
  
  const std::string kTableName = "MultipleThreadStatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  EXPECT_EQ(0, table.CountThreadsRegistered());

  
  
  
  StatsTableThread* threads[kMaxThreads];

  
  for (int index = 0; index < kMaxThreads; index++) {
    threads[index] = new StatsTableThread("MultipleThreadsTest", index);
    threads[index]->Start();
  }

  
  for (int index = 0; index < kMaxThreads; index++) {
    threads[index]->Join();
    delete threads[index];
  }

  StatsCounter zero_counter(kCounterZero);
  StatsCounter lucky13_counter(kCounter1313);
  StatsCounter increment_counter(kCounterIncrement);
  StatsCounter decrement_counter(kCounterDecrement);
  StatsCounter mixed_counter(kCounterMixed);

  
  std::string name;
  name = "c:" + kCounterZero;
  EXPECT_EQ(0, table.GetCounterValue(name));
  name = "c:" + kCounter1313;
  EXPECT_EQ(1313 * kMaxThreads,
      table.GetCounterValue(name));
  name = "c:" + kCounterIncrement;
  EXPECT_EQ(kMaxThreads * kThreadLoops,
      table.GetCounterValue(name));
  name = "c:" + kCounterDecrement;
  EXPECT_EQ(-kMaxThreads * kThreadLoops,
      table.GetCounterValue(name));
  name = "c:" + kCounterMixed;
  EXPECT_EQ((kMaxThreads % 2) * kThreadLoops,
      table.GetCounterValue(name));
  EXPECT_EQ(0, table.CountThreadsRegistered());

  DeleteShmem(kTableName);
#endif
}

const std::string kMPTableName = "MultipleProcessStatTable";

MULTIPROCESS_TEST_MAIN(StatsTableMultipleProcessMain) {
  
  
  

  StatsTable table(kMPTableName, 0, 0);
  StatsTable::set_current(&table);
  StatsCounter zero_counter(kCounterZero);
  StatsCounter lucky13_counter(kCounter1313);
  StatsCounter increment_counter(kCounterIncrement);
  StatsCounter decrement_counter(kCounterDecrement);
  for (int index = 0; index < kThreadLoops; index++) {
    zero_counter.Set(0);
    lucky13_counter.Set(1313);
    increment_counter.Increment();
    decrement_counter.Decrement();
    PlatformThread::Sleep(index % 10);   
  }
  return 0;
}


TEST_F(StatsTableTest, MultipleProcesses) {
  
  const int kMaxProcs = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kMPTableName);
  StatsTable table(kMPTableName, kMaxProcs, kMaxCounter);
  StatsTable::set_current(&table);
  EXPECT_EQ(0, table.CountThreadsRegistered());

  
  
  
  ProcessHandle procs[kMaxProcs];

  
  for (int16 index = 0; index < kMaxProcs; index++) {
    procs[index] = this->SpawnChild(L"StatsTableMultipleProcessMain");
    EXPECT_NE(static_cast<ProcessHandle>(NULL), procs[index]);
  }

  
  for (int index = 0; index < kMaxProcs; index++) {
    EXPECT_TRUE(WaitForSingleProcess(procs[index], 60 * 1000));
    base::CloseProcessHandle(procs[index]);
  }

  StatsCounter zero_counter(kCounterZero);
  StatsCounter lucky13_counter(kCounter1313);
  StatsCounter increment_counter(kCounterIncrement);
  StatsCounter decrement_counter(kCounterDecrement);

  
  std::string name;
  name = "c:" + kCounterZero;
  EXPECT_EQ(0, table.GetCounterValue(name));
  name = "c:" + kCounter1313;
  EXPECT_EQ(1313 * kMaxProcs,
      table.GetCounterValue(name));
  name = "c:" + kCounterIncrement;
  EXPECT_EQ(kMaxProcs * kThreadLoops,
      table.GetCounterValue(name));
  name = "c:" + kCounterDecrement;
  EXPECT_EQ(-kMaxProcs * kThreadLoops,
      table.GetCounterValue(name));
  EXPECT_EQ(0, table.CountThreadsRegistered());

  DeleteShmem(kMPTableName);
}

class MockStatsCounter : public StatsCounter {
 public:
  MockStatsCounter(const std::string& name)
      : StatsCounter(name) {}
  int* Pointer() { return GetPtr(); }
};


TEST_F(StatsTableTest, StatsCounter) {
  
  const std::string kTableName = "StatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  MockStatsCounter foo("foo");

  
  EXPECT_TRUE(foo.Enabled());
  EXPECT_NE(foo.Pointer(), static_cast<int*>(0));
  EXPECT_EQ(0, table.GetCounterValue("c:foo"));
  EXPECT_EQ(0, *(foo.Pointer()));

  
  while(*(foo.Pointer()) < 123) foo.Increment();
  EXPECT_EQ(123, table.GetCounterValue("c:foo"));
  foo.Add(0);
  EXPECT_EQ(123, table.GetCounterValue("c:foo"));
  foo.Add(-1);
  EXPECT_EQ(122, table.GetCounterValue("c:foo"));

  
  foo.Set(0);
  EXPECT_EQ(0, table.GetCounterValue("c:foo"));
  foo.Set(100);
  EXPECT_EQ(100, table.GetCounterValue("c:foo"));
  foo.Set(-1);
  EXPECT_EQ(-1, table.GetCounterValue("c:foo"));
  foo.Set(0);
  EXPECT_EQ(0, table.GetCounterValue("c:foo"));

  
  foo.Subtract(1);
  EXPECT_EQ(-1, table.GetCounterValue("c:foo"));
  foo.Subtract(0);
  EXPECT_EQ(-1, table.GetCounterValue("c:foo"));
  foo.Subtract(-1);
  EXPECT_EQ(0, table.GetCounterValue("c:foo"));

  DeleteShmem(kTableName);
}

class MockStatsCounterTimer : public StatsCounterTimer {
 public:
  MockStatsCounterTimer(const std::string& name)
      : StatsCounterTimer(name) {}

  TimeTicks start_time() { return start_time_; }
  TimeTicks stop_time() { return stop_time_; }
};


TEST_F(StatsTableTest, StatsCounterTimer) {
  
  const std::string kTableName = "StatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  MockStatsCounterTimer bar("bar");

  
  EXPECT_FALSE(bar.Running());
  EXPECT_TRUE(bar.start_time().is_null());
  EXPECT_TRUE(bar.stop_time().is_null());

  
  bar.Start();
  PlatformThread::Sleep(500);
  bar.Stop();
  EXPECT_LE(500, table.GetCounterValue("t:bar"));

  
  bar.Start();
  PlatformThread::Sleep(500);
  bar.Stop();
  EXPECT_LE(1000, table.GetCounterValue("t:bar"));
}


TEST_F(StatsTableTest, StatsRate) {
  
  const std::string kTableName = "StatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  StatsRate baz("baz");

  
  EXPECT_FALSE(baz.Running());
  EXPECT_EQ(0, table.GetCounterValue("c:baz"));
  EXPECT_EQ(0, table.GetCounterValue("t:baz"));

  
  baz.Start();
  PlatformThread::Sleep(500);
  baz.Stop();
  EXPECT_EQ(1, table.GetCounterValue("c:baz"));
  EXPECT_LE(500, table.GetCounterValue("t:baz"));

  
  baz.Start();
  PlatformThread::Sleep(500);
  baz.Stop();
  EXPECT_EQ(2, table.GetCounterValue("c:baz"));
  EXPECT_LE(1000, table.GetCounterValue("t:baz"));
}


TEST_F(StatsTableTest, StatsScope) {
  
  const std::string kTableName = "StatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  StatsCounterTimer foo("foo");
  StatsRate bar("bar");

  
  EXPECT_EQ(0, table.GetCounterValue("t:foo"));
  EXPECT_EQ(0, table.GetCounterValue("t:bar"));
  EXPECT_EQ(0, table.GetCounterValue("c:bar"));

  
  {
    StatsScope<StatsCounterTimer> timer(foo);
    StatsScope<StatsRate> timer2(bar);
    PlatformThread::Sleep(500);
  }
  EXPECT_LE(500, table.GetCounterValue("t:foo"));
  EXPECT_LE(500, table.GetCounterValue("t:bar"));
  EXPECT_EQ(1, table.GetCounterValue("c:bar"));

  
  {
    StatsScope<StatsCounterTimer> timer(foo);
    StatsScope<StatsRate> timer2(bar);
    PlatformThread::Sleep(500);
  }
  EXPECT_LE(1000, table.GetCounterValue("t:foo"));
  EXPECT_LE(1000, table.GetCounterValue("t:bar"));
  EXPECT_EQ(2, table.GetCounterValue("c:bar"));

  DeleteShmem(kTableName);
}

}  
