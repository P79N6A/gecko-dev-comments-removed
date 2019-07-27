









#ifdef HAVE_DBUS_GLIB

#include "webrtc/base/dbus.h"

#include <glib.h>

#include "webrtc/base/logging.h"
#include "webrtc/base/thread.h"

namespace rtc {


static pthread_once_t g_dbus_init_once = PTHREAD_ONCE_INIT;
static LibDBusGlibSymbolTable *g_dbus_symbol = NULL;


static void ReleaseDBusGlibSymbol() {
  if (g_dbus_symbol != NULL) {
    delete g_dbus_symbol;
    g_dbus_symbol = NULL;
  }
}


static void InitializeDBusGlibSymbol() {
  
  if (NULL == g_dbus_symbol) {
    g_dbus_symbol = new LibDBusGlibSymbolTable();

    
    if (NULL == g_dbus_symbol || !g_dbus_symbol->Load()) {
      LOG(LS_WARNING) << "Failed to load dbus-glib symbol table.";
      ReleaseDBusGlibSymbol();
    } else {
      
      atexit(ReleaseDBusGlibSymbol);
    }
  }
}

inline static LibDBusGlibSymbolTable *GetSymbols() {
  return DBusMonitor::GetDBusGlibSymbolTable();
}


DBusSigMessageData::DBusSigMessageData(DBusMessage *message)
    : TypedMessageData<DBusMessage *>(message) {
  GetSymbols()->dbus_message_ref()(data());
}

DBusSigMessageData::~DBusSigMessageData() {
  GetSymbols()->dbus_message_unref()(data());
}




std::string DBusSigFilter::BuildFilterString(const std::string &path,
                                             const std::string &interface,
                                             const std::string &member) {
  std::string ret(DBUS_TYPE "='" DBUS_SIGNAL "'");
  if (!path.empty()) {
    ret += ("," DBUS_PATH "='");
    ret += path;
    ret += "'";
  }
  if (!interface.empty()) {
    ret += ("," DBUS_INTERFACE "='");
    ret += interface;
    ret += "'";
  }
  if (!member.empty()) {
    ret += ("," DBUS_MEMBER "='");
    ret += member;
    ret += "'";
  }
  return ret;
}


DBusHandlerResult DBusSigFilter::DBusCallback(DBusConnection *dbus_conn,
                                              DBusMessage *message,
                                              void *instance) {
  ASSERT(instance);
  if (instance) {
    return static_cast<DBusSigFilter *>(instance)->Callback(message);
  }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


DBusHandlerResult DBusSigFilter::Callback(DBusMessage *message) {
  if (caller_thread_) {
    caller_thread_->Post(this, DSM_SIGNAL, new DBusSigMessageData(message));
  }
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


void DBusSigFilter::OnMessage(Message *message) {
  if (message != NULL && DSM_SIGNAL == message->message_id) {
    DBusSigMessageData *msg =
        static_cast<DBusSigMessageData *>(message->pdata);
    if (msg) {
      ProcessSignal(msg->data());
      delete msg;
    }
  }
}





class DBusMonitor::DBusMonitoringThread : public rtc::Thread {
 public:
  explicit DBusMonitoringThread(DBusMonitor *monitor,
                                GMainContext *context,
                                GMainLoop *mainloop,
                                std::vector<DBusSigFilter *> *filter_list)
      : monitor_(monitor),
        context_(context),
        mainloop_(mainloop),
        connection_(NULL),
        idle_source_(NULL),
        filter_list_(filter_list) {
    ASSERT(monitor_);
    ASSERT(context_);
    ASSERT(mainloop_);
    ASSERT(filter_list_);
  }

  virtual ~DBusMonitoringThread() {
    Stop();
  }

  
  virtual void Run() {
    ASSERT(NULL == connection_);

    
    monitor_->OnMonitoringStatusChanged(DMS_INITIALIZING);
    if (!Setup()) {
      LOG(LS_ERROR) << "DBus monitoring setup failed.";
      monitor_->OnMonitoringStatusChanged(DMS_FAILED);
      CleanUp();
      return;
    }
    monitor_->OnMonitoringStatusChanged(DMS_RUNNING);
    g_main_loop_run(mainloop_);
    monitor_->OnMonitoringStatusChanged(DMS_STOPPED);

    
    CleanUp();
    return;
  }

  
  virtual void Stop() {
    ASSERT(NULL == idle_source_);
    
    idle_source_ = g_idle_source_new();
    if (idle_source_) {
      g_source_set_callback(idle_source_, &Idle, this, NULL);
      g_source_attach(idle_source_, context_);
    } else {
      LOG(LS_ERROR) << "g_idle_source_new() failed.";
      QuitGMainloop();  
    }

    Thread::Stop();  
  }

 private:
  
  void RegisterAllFilters() {
    ASSERT(NULL != GetSymbols()->dbus_g_connection_get_connection()(
        connection_));

    for (std::vector<DBusSigFilter *>::iterator it = filter_list_->begin();
         it != filter_list_->end(); ++it) {
      DBusSigFilter *filter = (*it);
      if (!filter) {
        LOG(LS_ERROR) << "DBusSigFilter list corrupted.";
        continue;
      }

      GetSymbols()->dbus_bus_add_match()(
          GetSymbols()->dbus_g_connection_get_connection()(connection_),
          filter->filter().c_str(), NULL);

      if (!GetSymbols()->dbus_connection_add_filter()(
              GetSymbols()->dbus_g_connection_get_connection()(connection_),
              &DBusSigFilter::DBusCallback, filter, NULL)) {
        LOG(LS_ERROR) << "dbus_connection_add_filter() failed."
                      << "Filter: " << filter->filter();
        continue;
      }
    }
  }

  
  void UnRegisterAllFilters() {
    ASSERT(NULL != GetSymbols()->dbus_g_connection_get_connection()(
        connection_));

    for (std::vector<DBusSigFilter *>::iterator it = filter_list_->begin();
         it != filter_list_->end(); ++it) {
      DBusSigFilter *filter = (*it);
      if (!filter) {
        LOG(LS_ERROR) << "DBusSigFilter list corrupted.";
        continue;
      }
      GetSymbols()->dbus_connection_remove_filter()(
          GetSymbols()->dbus_g_connection_get_connection()(connection_),
          &DBusSigFilter::DBusCallback, filter);
    }
  }

  
  bool Setup() {
    g_main_context_push_thread_default(context_);

    
    
    connection_ = GetSymbols()->dbus_g_bus_get_private()(monitor_->type_,
        context_, NULL);
    if (NULL == connection_) {
      LOG(LS_ERROR) << "dbus_g_bus_get_private() unable to get connection.";
      return false;
    }
    if (NULL == GetSymbols()->dbus_g_connection_get_connection()(connection_)) {
      LOG(LS_ERROR) << "dbus_g_connection_get_connection() returns NULL. "
                    << "DBus daemon is probably not running.";
      return false;
    }

    
    GetSymbols()->dbus_connection_set_exit_on_disconnect()(
        GetSymbols()->dbus_g_connection_get_connection()(connection_), FALSE);

    
    RegisterAllFilters();

    return true;
  }

  
  void CleanUp() {
    if (idle_source_) {
      
      g_source_destroy(idle_source_);
      
      g_source_unref(idle_source_);
      idle_source_ = NULL;
    }
    if (connection_) {
      if (GetSymbols()->dbus_g_connection_get_connection()(connection_)) {
        UnRegisterAllFilters();
        GetSymbols()->dbus_connection_close()(
            GetSymbols()->dbus_g_connection_get_connection()(connection_));
      }
      GetSymbols()->dbus_g_connection_unref()(connection_);
      connection_ = NULL;
    }
    g_main_loop_unref(mainloop_);
    mainloop_ = NULL;
    g_main_context_unref(context_);
    context_ = NULL;
  }

  
  static gboolean Idle(gpointer data) {
    static_cast<DBusMonitoringThread *>(data)->QuitGMainloop();
    return TRUE;
  }

  
  void QuitGMainloop() {
    g_main_loop_quit(mainloop_);
  }

  DBusMonitor *monitor_;

  GMainContext *context_;
  GMainLoop *mainloop_;
  DBusGConnection *connection_;
  GSource *idle_source_;

  std::vector<DBusSigFilter *> *filter_list_;
};




LibDBusGlibSymbolTable *DBusMonitor::GetDBusGlibSymbolTable() {
  
  pthread_once(&g_dbus_init_once, InitializeDBusGlibSymbol);

  return g_dbus_symbol;
};


DBusMonitor *DBusMonitor::Create(DBusBusType type) {
  if (NULL == DBusMonitor::GetDBusGlibSymbolTable()) {
    return NULL;
  }
  return new DBusMonitor(type);
}

DBusMonitor::DBusMonitor(DBusBusType type)
    : type_(type),
      status_(DMS_NOT_INITIALIZED),
      monitoring_thread_(NULL) {
  ASSERT(type_ == DBUS_BUS_SYSTEM || type_ == DBUS_BUS_SESSION);
}

DBusMonitor::~DBusMonitor() {
  StopMonitoring();
}

bool DBusMonitor::AddFilter(DBusSigFilter *filter) {
  if (monitoring_thread_) {
    return false;
  }
  if (!filter) {
    return false;
  }
  filter_list_.push_back(filter);
  return true;
}

bool DBusMonitor::StartMonitoring() {
  if (!monitoring_thread_) {
    g_type_init();
    g_thread_init(NULL);
    GetSymbols()->dbus_g_thread_init()();

    GMainContext *context = g_main_context_new();
    if (NULL == context) {
      LOG(LS_ERROR) << "g_main_context_new() failed.";
      return false;
    }

    GMainLoop *mainloop = g_main_loop_new(context, FALSE);
    if (NULL == mainloop) {
      LOG(LS_ERROR) << "g_main_loop_new() failed.";
      g_main_context_unref(context);
      return false;
    }

    monitoring_thread_ = new DBusMonitoringThread(this, context, mainloop,
                                                  &filter_list_);
    if (monitoring_thread_ == NULL) {
      LOG(LS_ERROR) << "Failed to create DBus monitoring thread.";
      g_main_context_unref(context);
      g_main_loop_unref(mainloop);
      return false;
    }
    monitoring_thread_->Start();
  }
  return true;
}

bool DBusMonitor::StopMonitoring() {
  if (monitoring_thread_) {
    monitoring_thread_->Stop();
    monitoring_thread_ = NULL;
  }
  return true;
}

DBusMonitor::DBusMonitorStatus DBusMonitor::GetStatus() {
  return status_;
}

void DBusMonitor::OnMonitoringStatusChanged(DBusMonitorStatus status) {
  status_ = status;
}

#undef LATE

}  

#endif  
