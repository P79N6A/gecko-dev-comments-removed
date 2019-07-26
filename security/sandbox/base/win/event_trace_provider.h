





#ifndef BASE_WIN_EVENT_TRACE_PROVIDER_H_
#define BASE_WIN_EVENT_TRACE_PROVIDER_H_

#include <windows.h>
#include <wmistr.h>
#include <evntrace.h>

#include "base/base_export.h"
#include "base/basictypes.h"

namespace base {
namespace win {

typedef GUID EtwEventClass;
typedef UCHAR EtwEventType;
typedef UCHAR EtwEventLevel;
typedef USHORT EtwEventVersion;
typedef ULONG EtwEventFlags;


template <size_t N> struct EtwMofEventBase {
  EVENT_TRACE_HEADER header;
  MOF_FIELD fields[N];
};


template <size_t N> class EtwMofEvent: public EtwMofEventBase<N> {
 public:
  typedef EtwMofEventBase<N> Super;

  
  
  using EtwMofEventBase<N>::header;
  using EtwMofEventBase<N>::fields;

  EtwMofEvent() {
    memset(static_cast<Super*>(this), 0, sizeof(Super));
  }

  EtwMofEvent(const EtwEventClass& event_class, EtwEventType type,
              EtwEventLevel level) {
    memset(static_cast<Super*>(this), 0, sizeof(Super));
    header.Size = sizeof(Super);
    header.Guid = event_class;
    header.Class.Type = type;
    header.Class.Level = level;
    header.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
  }

  EtwMofEvent(const EtwEventClass& event_class, EtwEventType type,
              EtwEventVersion version, EtwEventLevel level) {
    memset(static_cast<Super*>(this), 0, sizeof(Super));
    header.Size = sizeof(Super);
    header.Guid = event_class;
    header.Class.Type = type;
    header.Class.Version = version;
    header.Class.Level = level;
    header.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
  }

  void SetField(int field, size_t size, const void *data) {
    
    if ((field < N) && (size <= kuint32max)) {
      fields[field].DataPtr = reinterpret_cast<ULONG64>(data);
      fields[field].Length = static_cast<ULONG>(size);
    }
  }

  EVENT_TRACE_HEADER* get() { return& header; }

 private:
  DISALLOW_COPY_AND_ASSIGN(EtwMofEvent);
};









class BASE_EXPORT EtwTraceProvider {
 public:
  
  
  explicit EtwTraceProvider(const GUID& provider_name);

  
  
  EtwTraceProvider();
  virtual ~EtwTraceProvider();

  
  
  
  
  
  ULONG Register();
  
  ULONG Unregister();

  
  void set_provider_name(const GUID& provider_name) {
    provider_name_ = provider_name;
  }
  const GUID& provider_name() const { return provider_name_; }
  TRACEHANDLE registration_handle() const { return registration_handle_; }
  TRACEHANDLE session_handle() const { return session_handle_; }
  EtwEventFlags enable_flags() const { return enable_flags_; }
  EtwEventLevel enable_level() const { return enable_level_; }

  
  
  
  bool ShouldLog(EtwEventLevel level, EtwEventFlags flags) {
    return NULL != session_handle_ && level >= enable_level_ &&
        (0 != (flags & enable_flags_));
  }

  
  
  ULONG Log(const EtwEventClass& event_class, EtwEventType type,
            EtwEventLevel level, const char *message);
  ULONG Log(const EtwEventClass& event_class, EtwEventType type,
            EtwEventLevel level, const wchar_t *message);

  
  ULONG Log(EVENT_TRACE_HEADER* event);

 protected:
  
  
  
  
  virtual void OnEventsEnabled() {}

  
  
  
  
  virtual void OnEventsDisabled() {}

  
  
  
  
  
  virtual void PostEventsDisabled() {}

 private:
  ULONG EnableEvents(PVOID buffer);
  ULONG DisableEvents();
  ULONG Callback(WMIDPREQUESTCODE request, PVOID buffer);
  static ULONG WINAPI ControlCallback(WMIDPREQUESTCODE request, PVOID context,
                                      ULONG *reserved, PVOID buffer);

  GUID provider_name_;
  TRACEHANDLE registration_handle_;
  TRACEHANDLE session_handle_;
  EtwEventFlags enable_flags_;
  EtwEventLevel enable_level_;

  
  
  static TRACE_GUID_REGISTRATION obligatory_guid_registration_;

  DISALLOW_COPY_AND_ASSIGN(EtwTraceProvider);
};

}  
}  

#endif  
