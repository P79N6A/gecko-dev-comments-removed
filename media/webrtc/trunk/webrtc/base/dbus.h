









#ifndef WEBRTC_BASE_DBUS_H_
#define WEBRTC_BASE_DBUS_H_

#ifdef HAVE_DBUS_GLIB

#include <dbus/dbus.h>

#include <string>
#include <vector>

#include "webrtc/base/libdbusglibsymboltable.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/thread.h"

namespace rtc {

#define DBUS_TYPE                   "type"
#define DBUS_SIGNAL                 "signal"
#define DBUS_PATH                   "path"
#define DBUS_INTERFACE              "interface"
#define DBUS_MEMBER                 "member"

#ifdef CHROMEOS
#define CROS_PM_PATH                "/"
#define CROS_PM_INTERFACE           "org.chromium.PowerManager"
#define CROS_SIG_POWERCHANGED       "PowerStateChanged"
#define CROS_VALUE_SLEEP            "mem"
#define CROS_VALUE_RESUME           "on"
#else
#define UP_PATH                     "/org/freedesktop/UPower"
#define UP_INTERFACE                "org.freedesktop.UPower"
#define UP_SIG_SLEEPING             "Sleeping"
#define UP_SIG_RESUMING             "Resuming"
#endif  


class DBusSigMessageData : public TypedMessageData<DBusMessage *> {
 public:
  explicit DBusSigMessageData(DBusMessage *message);
  ~DBusSigMessageData();
};






class DBusSigFilter : protected MessageHandler {
 public:
  enum DBusSigMessage { DSM_SIGNAL };

  
  explicit DBusSigFilter(const std::string &filter)
      : caller_thread_(Thread::Current()), filter_(filter) {
  }

  
  
  
  static std::string BuildFilterString(const std::string &path,
                                       const std::string &interface,
                                       const std::string &member);

  
  static DBusHandlerResult DBusCallback(DBusConnection *dbus_conn,
                                        DBusMessage *message,
                                        void *instance);

  
  DBusHandlerResult Callback(DBusMessage *message);

  
  virtual void OnMessage(Message *message);

  
  const std::string &filter() const { return filter_; }

 private:
  
  virtual void ProcessSignal(DBusMessage *message) = 0;

  Thread *caller_thread_;
  const std::string filter_;
};























class DBusMonitor {
 public:
  
  enum DBusMonitorStatus {
    DMS_NOT_INITIALIZED,  
    DMS_INITIALIZING,     
    DMS_RUNNING,          
    DMS_STOPPED,          
    DMS_FAILED,           
  };

  
  
  static LibDBusGlibSymbolTable *GetDBusGlibSymbolTable();

  
  static DBusMonitor *Create(DBusBusType type);
  ~DBusMonitor();

  
  bool AddFilter(DBusSigFilter *filter);

  
  bool StartMonitoring();

  
  bool StopMonitoring();

  
  DBusMonitorStatus GetStatus();

 private:
  
  class DBusMonitoringThread;

  explicit DBusMonitor(DBusBusType type);

  
  void OnMonitoringStatusChanged(DBusMonitorStatus status);

  DBusBusType type_;
  DBusMonitorStatus status_;
  DBusMonitoringThread *monitoring_thread_;
  std::vector<DBusSigFilter *> filter_list_;
};

}  

#endif

#endif
