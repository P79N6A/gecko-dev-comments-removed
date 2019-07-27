









#include <iomanip>
#include <iostream>
#include <vector>

#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#endif

#include "webrtc/base/cpumonitor.h"
#include "webrtc/base/flags.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/base/timing.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace rtc {

static const int kMaxCpus = 1024;
static const int kSettleTime = 100;  
static const int kIdleTime = 500;  
static const int kBusyTime = 1000;  
static const int kLongInterval = 2000;  

class BusyThread : public rtc::Thread {
 public:
  BusyThread(double load, double duration, double interval) :
    load_(load), duration_(duration), interval_(interval) {
  }
  virtual ~BusyThread() {
    Stop();
  }
  void Run() {
    Timing time;
    double busy_time = interval_ * load_ / 100.0;
    for (;;) {
      time.BusyWait(busy_time);
      time.IdleWait(interval_ - busy_time);
      if (duration_) {
        duration_ -= interval_;
        if (duration_ <= 0) {
          break;
        }
      }
    }
  }
 private:
  double load_;
  double duration_;
  double interval_;
};

class CpuLoadListener : public sigslot::has_slots<> {
 public:
  CpuLoadListener()
      : current_cpus_(0),
        cpus_(0),
        process_load_(.0f),
        system_load_(.0f),
        count_(0) {
  }

  void OnCpuLoad(int current_cpus, int cpus, float proc_load, float sys_load) {
    current_cpus_ = current_cpus;
    cpus_ = cpus;
    process_load_ = proc_load;
    system_load_ = sys_load;
    ++count_;
  }

  int current_cpus() const { return current_cpus_; }
  int cpus() const { return cpus_; }
  float process_load() const { return process_load_; }
  float system_load() const { return system_load_; }
  int count() const { return count_; }

 private:
  int current_cpus_;
  int cpus_;
  float process_load_;
  float system_load_;
  int count_;
};








bool SetThreadAffinity(BusyThread* t, int cpu, int affinity) {
#if defined(WEBRTC_WIN)
  if (affinity >= 0) {
    return ::SetThreadAffinityMask(t->GetHandle(),
        1 << (cpu + affinity)) != FALSE;
  }
#endif
  return true;
}

bool SetThreadPriority(BusyThread* t, int prio) {
  if (!prio) {
    return true;
  }
  bool ok = t->SetPriority(static_cast<rtc::ThreadPriority>(prio));
  if (!ok) {
    std::cout << "Error setting thread priority." << std::endl;
  }
  return ok;
}

int CpuLoad(double cpuload, double duration, int numthreads,
            int priority, double interval, int affinity) {
  int ret = 0;
  std::vector<BusyThread*> threads;
  for (int i = 0; i < numthreads; ++i) {
    threads.push_back(new BusyThread(cpuload, duration, interval));
    
    if (!SetThreadPriority(threads[i], priority) ||
       !threads[i]->Start() ||
       !SetThreadAffinity(threads[i], i, affinity)) {
      ret = 1;
      break;
    }
  }
  
  if (ret == 0) {
    for (int i = 0; i < numthreads; ++i) {
      threads[i]->Stop();
    }
  }

  for (int i = 0; i < numthreads; ++i) {
    delete threads[i];
  }
  return ret;
}


static void CpuTwoBusyLoop(int busytime) {
  CpuLoad(100.0, busytime / 1000.0, 2, 1, 0.050, -1);
}


static void CpuBusyLoop(int busytime) {
  CpuLoad(100.0, busytime / 1000.0, 1, 1, 0.050, -1);
}


static void CpuHalfBusyLoop(int busytime) {
  CpuLoad(50.0, busytime / 1000.0, 1, 1, 0.050, -1);
}

void TestCpuSampler(bool test_proc, bool test_sys, bool force_fallback) {
  CpuSampler sampler;
  sampler.set_force_fallback(force_fallback);
  EXPECT_TRUE(sampler.Init());
  sampler.set_load_interval(100);
  int cpus = sampler.GetMaxCpus();

  
  Thread::SleepMs(kSettleTime);
  sampler.GetProcessLoad();
  sampler.GetSystemLoad();

  Thread::SleepMs(kIdleTime);

  float proc_idle = 0.f, sys_idle = 0.f;
  if (test_proc) {
    proc_idle = sampler.GetProcessLoad();
  }
  if (test_sys) {
      sys_idle = sampler.GetSystemLoad();
  }
  if (test_proc) {
    LOG(LS_INFO) << "ProcessLoad Idle:      "
                 << std::setiosflags(std::ios_base::fixed)
                 << std::setprecision(2) << std::setw(6) << proc_idle;
    EXPECT_GE(proc_idle, 0.f);
    EXPECT_LE(proc_idle, static_cast<float>(cpus));
  }
  if (test_sys) {
    LOG(LS_INFO) << "SystemLoad Idle:       "
                 << std::setiosflags(std::ios_base::fixed)
                 << std::setprecision(2) << std::setw(6) << sys_idle;
    EXPECT_GE(sys_idle, 0.f);
    EXPECT_LE(sys_idle, static_cast<float>(cpus));
  }

  
  Thread::SleepMs(kSettleTime);
  sampler.GetProcessLoad();
  sampler.GetSystemLoad();

  CpuHalfBusyLoop(kBusyTime);

  float proc_halfbusy = 0.f, sys_halfbusy = 0.f;
  if (test_proc) {
    proc_halfbusy = sampler.GetProcessLoad();
  }
  if (test_sys) {
    sys_halfbusy = sampler.GetSystemLoad();
  }
  if (test_proc) {
    LOG(LS_INFO) << "ProcessLoad Halfbusy:  "
                 << std::setiosflags(std::ios_base::fixed)
                 << std::setprecision(2) << std::setw(6) << proc_halfbusy;
    EXPECT_GE(proc_halfbusy, 0.f);
    EXPECT_LE(proc_halfbusy, static_cast<float>(cpus));
  }
  if (test_sys) {
    LOG(LS_INFO) << "SystemLoad Halfbusy:   "
                 << std::setiosflags(std::ios_base::fixed)
                 << std::setprecision(2) << std::setw(6) << sys_halfbusy;
    EXPECT_GE(sys_halfbusy, 0.f);
    EXPECT_LE(sys_halfbusy, static_cast<float>(cpus));
  }

  
  Thread::SleepMs(kSettleTime);
  sampler.GetProcessLoad();
  sampler.GetSystemLoad();

  CpuBusyLoop(kBusyTime);

  float proc_busy = 0.f, sys_busy = 0.f;
  if (test_proc) {
    proc_busy = sampler.GetProcessLoad();
  }
  if (test_sys) {
    sys_busy = sampler.GetSystemLoad();
  }
  if (test_proc) {
    LOG(LS_INFO) << "ProcessLoad Busy:      "
                 << std::setiosflags(std::ios_base::fixed)
                 << std::setprecision(2) << std::setw(6) << proc_busy;
    EXPECT_GE(proc_busy, 0.f);
    EXPECT_LE(proc_busy, static_cast<float>(cpus));
  }
  if (test_sys) {
    LOG(LS_INFO) << "SystemLoad Busy:       "
                 << std::setiosflags(std::ios_base::fixed)
                 << std::setprecision(2) << std::setw(6) << sys_busy;
    EXPECT_GE(sys_busy, 0.f);
    EXPECT_LE(sys_busy, static_cast<float>(cpus));
  }

  
  if (cpus >= 2) {
    Thread::SleepMs(kSettleTime);
    sampler.GetProcessLoad();
    sampler.GetSystemLoad();

    CpuTwoBusyLoop(kBusyTime);

    float proc_twobusy = 0.f, sys_twobusy = 0.f;
    if (test_proc) {
      proc_twobusy = sampler.GetProcessLoad();
    }
    if (test_sys) {
      sys_twobusy = sampler.GetSystemLoad();
    }
    if (test_proc) {
      LOG(LS_INFO) << "ProcessLoad 2 CPU Busy:"
                   << std::setiosflags(std::ios_base::fixed)
                   << std::setprecision(2) << std::setw(6) << proc_twobusy;
      EXPECT_GE(proc_twobusy, 0.f);
      EXPECT_LE(proc_twobusy, static_cast<float>(cpus));
    }
    if (test_sys) {
      LOG(LS_INFO) << "SystemLoad 2 CPU Busy: "
                   << std::setiosflags(std::ios_base::fixed)
                   << std::setprecision(2) << std::setw(6) << sys_twobusy;
      EXPECT_GE(sys_twobusy, 0.f);
      EXPECT_LE(sys_twobusy, static_cast<float>(cpus));
    }
  }

  
  Thread::SleepMs(kSettleTime);
  sampler.GetProcessLoad();
  sampler.GetSystemLoad();

  Thread::SleepMs(kIdleTime);

  if (test_proc) {
    proc_idle = sampler.GetProcessLoad();
  }
  if (test_sys) {
    sys_idle = sampler.GetSystemLoad();
  }
  if (test_proc) {
    LOG(LS_INFO) << "ProcessLoad Idle:      "
                 << std::setiosflags(std::ios_base::fixed)
                 << std::setprecision(2) << std::setw(6) << proc_idle;
    EXPECT_GE(proc_idle, 0.f);
    EXPECT_LE(proc_idle, proc_busy);
  }
  if (test_sys) {
    LOG(LS_INFO) << "SystemLoad Idle:       "
                 << std::setiosflags(std::ios_base::fixed)
                 << std::setprecision(2) << std::setw(6) << sys_idle;
    EXPECT_GE(sys_idle, 0.f);
    EXPECT_LE(sys_idle, static_cast<float>(cpus));
  }
}

TEST(CpuMonitorTest, TestCpus) {
  CpuSampler sampler;
  EXPECT_TRUE(sampler.Init());
  int current_cpus = sampler.GetCurrentCpus();
  int cpus = sampler.GetMaxCpus();
  LOG(LS_INFO) << "Current Cpus:     " << std::setw(9) << current_cpus;
  LOG(LS_INFO) << "Maximum Cpus:     " << std::setw(9) << cpus;
  EXPECT_GT(cpus, 0);
  EXPECT_LE(cpus, kMaxCpus);
  EXPECT_GT(current_cpus, 0);
  EXPECT_LE(current_cpus, cpus);
}

#if defined(WEBRTC_WIN)

TEST(CpuMonitorTest, TestGetSystemLoadForceFallback) {
  TestCpuSampler(false, true, true);
}
#endif


TEST(CpuMonitorTest, TestGetBothLoad) {
  TestCpuSampler(true, true, false);
}


TEST(CpuMonitorTest, TestInterval) {
  CpuSampler sampler;
  EXPECT_TRUE(sampler.Init());

  
  sampler.set_load_interval(kLongInterval);

  sampler.GetProcessLoad();
  sampler.GetSystemLoad();

  float proc_orig = sampler.GetProcessLoad();
  float sys_orig = sampler.GetSystemLoad();

  Thread::SleepMs(kIdleTime);

  float proc_halftime = sampler.GetProcessLoad();
  float sys_halftime = sampler.GetSystemLoad();

  EXPECT_EQ(proc_orig, proc_halftime);
  EXPECT_EQ(sys_orig, sys_halftime);
}

TEST(CpuMonitorTest, TestCpuMonitor) {
  CpuMonitor monitor(Thread::Current());
  CpuLoadListener listener;
  monitor.SignalUpdate.connect(&listener, &CpuLoadListener::OnCpuLoad);
  EXPECT_TRUE(monitor.Start(10));
  
  EXPECT_TRUE_WAIT(listener.count() > 2, 1000);
  EXPECT_GT(listener.current_cpus(), 0);
  EXPECT_GT(listener.cpus(), 0);
  EXPECT_GE(listener.process_load(), .0f);
  EXPECT_GE(listener.system_load(), .0f);

  monitor.Stop();
  
  Thread::Current()->ProcessMessages(20);
  int old_count = listener.count();
  Thread::Current()->ProcessMessages(20);
  
  EXPECT_EQ(old_count, listener.count());
}

}  
