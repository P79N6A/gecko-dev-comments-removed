









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_IMPL_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_TRACE_IMPL_H_

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/file_wrapper.h"
#include "webrtc/system_wrappers/interface/static_instance.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {





#if defined(WEBRTC_IOS)
#define WEBRTC_TRACE_MAX_QUEUE  2000
#else
#define WEBRTC_TRACE_MAX_QUEUE  8000
#endif
#define WEBRTC_TRACE_NUM_ARRAY 2
#define WEBRTC_TRACE_MAX_MESSAGE_SIZE 256





#define WEBRTC_TRACE_MAX_FILE_SIZE 100*1000




class TraceImpl : public Trace {
 public:
  virtual ~TraceImpl();

  static TraceImpl* CreateInstance();
  static TraceImpl* GetTrace(const TraceLevel level = kTraceAll);

  WebRtc_Word32 SetTraceFileImpl(const char* file_name,
                                 const bool add_file_counter);
  WebRtc_Word32 TraceFileImpl(
    char file_name[FileWrapper::kMaxFileNameSize]);

  WebRtc_Word32 SetTraceCallbackImpl(TraceCallback* callback);

  void AddImpl(const TraceLevel level, const TraceModule module,
               const WebRtc_Word32 id, const char* msg);

  bool StopThread();

  bool TraceCheck(const TraceLevel level) const;

 protected:
  TraceImpl();

  static TraceImpl* StaticInstance(CountOperation count_operation,
                                   const TraceLevel level = kTraceAll);

  WebRtc_Word32 AddThreadId(char* trace_message) const;

  
  virtual WebRtc_Word32 AddTime(char* trace_message,
                                const TraceLevel level) const = 0;

  virtual WebRtc_Word32 AddBuildInfo(char* trace_message) const = 0;
  virtual WebRtc_Word32 AddDateTimeInfo(char* trace_message) const = 0;

  static bool Run(void* obj);
  bool Process();

 private:
  friend class Trace;

  WebRtc_Word32 AddLevel(char* sz_message, const TraceLevel level) const;

  WebRtc_Word32 AddModuleAndId(char* trace_message, const TraceModule module,
                               const WebRtc_Word32 id) const;

  WebRtc_Word32 AddMessage(char* trace_message,
                           const char msg[WEBRTC_TRACE_MAX_MESSAGE_SIZE],
                           const WebRtc_UWord16 written_so_far) const;

  void AddMessageToList(
    const char trace_message[WEBRTC_TRACE_MAX_MESSAGE_SIZE],
    const WebRtc_UWord16 length,
    const TraceLevel level);

  bool UpdateFileName(
    const char file_name_utf8[FileWrapper::kMaxFileNameSize],
    char file_name_with_counter_utf8[FileWrapper::kMaxFileNameSize],
    const WebRtc_UWord32 new_count) const;

  bool CreateFileName(
    const char file_name_utf8[FileWrapper::kMaxFileNameSize],
    char file_name_with_counter_utf8[FileWrapper::kMaxFileNameSize],
    const WebRtc_UWord32 new_count) const;

  void WriteToFile();

  CriticalSectionWrapper* critsect_interface_;
  TraceCallback* callback_;
  WebRtc_UWord32 row_count_text_;
  WebRtc_UWord32 file_count_text_;

  FileWrapper& trace_file_;
  ThreadWrapper& thread_;
  EventWrapper& event_;

  
  CriticalSectionWrapper* critsect_array_;
  WebRtc_UWord16 next_free_idx_[WEBRTC_TRACE_NUM_ARRAY];
  TraceLevel level_[WEBRTC_TRACE_NUM_ARRAY][WEBRTC_TRACE_MAX_QUEUE];
  WebRtc_UWord16 length_[WEBRTC_TRACE_NUM_ARRAY][WEBRTC_TRACE_MAX_QUEUE];
  char* message_queue_[WEBRTC_TRACE_NUM_ARRAY][WEBRTC_TRACE_MAX_QUEUE];
  WebRtc_UWord8 active_queue_;
};

}  

#endif  
