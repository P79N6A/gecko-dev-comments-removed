









#ifdef HAVE_DBUS_GLIB

#include "webrtc/base/dbus.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/thread.h"

namespace rtc {

#define SIG_NAME "NameAcquired"

static const uint32 kTimeoutMs = 5000U;

class DBusSigFilterTest : public DBusSigFilter {
 public:
  
  
  
  
  DBusSigFilterTest()
      : DBusSigFilter(GetFilter()),
        message_received_(false) {
  }

  bool MessageReceived() {
    return message_received_;
  }

 private:
  static std::string GetFilter() {
    return rtc::DBusSigFilter::BuildFilterString("", "", SIG_NAME);
  }

  
  virtual void ProcessSignal(DBusMessage *message) {
    EXPECT_TRUE(message != NULL);
    message_received_ = true;
  }

  bool message_received_;
};

TEST(DBusMonitorTest, StartStopStartStop) {
  DBusSigFilterTest filter;
  rtc::scoped_ptr<rtc::DBusMonitor> monitor;
  monitor.reset(rtc::DBusMonitor::Create(DBUS_BUS_SYSTEM));
  if (monitor) {
    EXPECT_TRUE(monitor->AddFilter(&filter));

    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_NOT_INITIALIZED);

    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor->GetStatus(), kTimeoutMs);
    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_STOPPED);
    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_STOPPED);

    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor->GetStatus(), kTimeoutMs);
    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_RUNNING);
    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_STOPPED);
  } else {
    LOG(LS_WARNING) << "DBus Monitor not started. Skipping test.";
  }
}





TEST(DBusMonitorTest, ReceivedNameAcquiredSignal) {
  DBusSigFilterTest filter;
  rtc::scoped_ptr<rtc::DBusMonitor> monitor;
  monitor.reset(rtc::DBusMonitor::Create(DBUS_BUS_SYSTEM));
  if (monitor) {
    EXPECT_TRUE(monitor->AddFilter(&filter));

    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor->GetStatus(), kTimeoutMs);
    EXPECT_TRUE_WAIT(filter.MessageReceived(), kTimeoutMs);
    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_STOPPED);
  } else {
    LOG(LS_WARNING) << "DBus Monitor not started. Skipping test.";
  }
}

TEST(DBusMonitorTest, ConcurrentMonitors) {
  DBusSigFilterTest filter1;
  rtc::scoped_ptr<rtc::DBusMonitor> monitor1;
  monitor1.reset(rtc::DBusMonitor::Create(DBUS_BUS_SYSTEM));
  if (monitor1) {
    EXPECT_TRUE(monitor1->AddFilter(&filter1));
    DBusSigFilterTest filter2;
    rtc::scoped_ptr<rtc::DBusMonitor> monitor2;
    monitor2.reset(rtc::DBusMonitor::Create(DBUS_BUS_SYSTEM));
    EXPECT_TRUE(monitor2->AddFilter(&filter2));

    EXPECT_TRUE(monitor1->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor1->GetStatus(), kTimeoutMs);
    EXPECT_TRUE(monitor2->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor2->GetStatus(), kTimeoutMs);

    EXPECT_TRUE_WAIT(filter2.MessageReceived(), kTimeoutMs);
    EXPECT_TRUE(monitor2->StopMonitoring());
    EXPECT_EQ(monitor2->GetStatus(), DBusMonitor::DMS_STOPPED);

    EXPECT_TRUE_WAIT(filter1.MessageReceived(), kTimeoutMs);
    EXPECT_TRUE(monitor1->StopMonitoring());
    EXPECT_EQ(monitor1->GetStatus(), DBusMonitor::DMS_STOPPED);
  } else {
    LOG(LS_WARNING) << "DBus Monitor not started. Skipping test.";
  }
}

TEST(DBusMonitorTest, ConcurrentFilters) {
  DBusSigFilterTest filter1;
  DBusSigFilterTest filter2;
  rtc::scoped_ptr<rtc::DBusMonitor> monitor;
  monitor.reset(rtc::DBusMonitor::Create(DBUS_BUS_SYSTEM));
  if (monitor) {
    EXPECT_TRUE(monitor->AddFilter(&filter1));
    EXPECT_TRUE(monitor->AddFilter(&filter2));

    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor->GetStatus(), kTimeoutMs);

    EXPECT_TRUE_WAIT(filter1.MessageReceived(), kTimeoutMs);
    EXPECT_TRUE_WAIT(filter2.MessageReceived(), kTimeoutMs);

    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_STOPPED);
  } else {
    LOG(LS_WARNING) << "DBus Monitor not started. Skipping test.";
  }
}

TEST(DBusMonitorTest, NoAddFilterIfRunning) {
  DBusSigFilterTest filter1;
  DBusSigFilterTest filter2;
  rtc::scoped_ptr<rtc::DBusMonitor> monitor;
  monitor.reset(rtc::DBusMonitor::Create(DBUS_BUS_SYSTEM));
  if (monitor) {
    EXPECT_TRUE(monitor->AddFilter(&filter1));

    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor->GetStatus(), kTimeoutMs);
    EXPECT_FALSE(monitor->AddFilter(&filter2));

    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_STOPPED);
  } else {
    LOG(LS_WARNING) << "DBus Monitor not started. Skipping test.";
  }
}

TEST(DBusMonitorTest, AddFilterAfterStop) {
  DBusSigFilterTest filter1;
  DBusSigFilterTest filter2;
  rtc::scoped_ptr<rtc::DBusMonitor> monitor;
  monitor.reset(rtc::DBusMonitor::Create(DBUS_BUS_SYSTEM));
  if (monitor) {
    EXPECT_TRUE(monitor->AddFilter(&filter1));
    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor->GetStatus(), kTimeoutMs);
    EXPECT_TRUE_WAIT(filter1.MessageReceived(), kTimeoutMs);
    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_STOPPED);

    EXPECT_TRUE(monitor->AddFilter(&filter2));
    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_EQ_WAIT(DBusMonitor::DMS_RUNNING, monitor->GetStatus(), kTimeoutMs);
    EXPECT_TRUE_WAIT(filter1.MessageReceived(), kTimeoutMs);
    EXPECT_TRUE_WAIT(filter2.MessageReceived(), kTimeoutMs);
    EXPECT_TRUE(monitor->StopMonitoring());
    EXPECT_EQ(monitor->GetStatus(), DBusMonitor::DMS_STOPPED);
  } else {
    LOG(LS_WARNING) << "DBus Monitor not started. Skipping test.";
  }
}

TEST(DBusMonitorTest, StopRightAfterStart) {
  DBusSigFilterTest filter;
  rtc::scoped_ptr<rtc::DBusMonitor> monitor;
  monitor.reset(rtc::DBusMonitor::Create(DBUS_BUS_SYSTEM));
  if (monitor) {
    EXPECT_TRUE(monitor->AddFilter(&filter));

    EXPECT_TRUE(monitor->StartMonitoring());
    EXPECT_TRUE(monitor->StopMonitoring());

    
    
    
    
    
    
    rtc::Thread::Current()->ProcessMessages(1);
  } else {
    LOG(LS_WARNING) << "DBus Monitor not started.";
  }
}

TEST(DBusSigFilter, BuildFilterString) {
  EXPECT_EQ(DBusSigFilter::BuildFilterString("", "", ""),
      (DBUS_TYPE "='" DBUS_SIGNAL "'"));
  EXPECT_EQ(DBusSigFilter::BuildFilterString("p", "", ""),
      (DBUS_TYPE "='" DBUS_SIGNAL "'," DBUS_PATH "='p'"));
  EXPECT_EQ(DBusSigFilter::BuildFilterString("p","i", ""),
      (DBUS_TYPE "='" DBUS_SIGNAL "'," DBUS_PATH "='p',"
          DBUS_INTERFACE "='i'"));
  EXPECT_EQ(DBusSigFilter::BuildFilterString("p","i","m"),
      (DBUS_TYPE "='" DBUS_SIGNAL "'," DBUS_PATH "='p',"
          DBUS_INTERFACE "='i'," DBUS_MEMBER "='m'"));
}

}  

#endif  
