









#include "webrtc/system_wrappers/source/trace_impl.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include "webrtc/system_wrappers/source/trace_win.h"
#else
#include "webrtc/system_wrappers/source/trace_posix.h"
#endif  

#include "webrtc/system_wrappers/interface/sleep.h"

#define KEY_LEN_CHARS 31

#ifdef _WIN32
#pragma warning(disable:4355)
#endif  

namespace webrtc {

const int Trace::kBoilerplateLength = 71;
const int Trace::kTimestampPosition = 13;
const int Trace::kTimestampLength = 12;
uint32_t Trace::level_filter_ = kTraceDefault;


TraceImpl* TraceImpl::StaticInstance(CountOperation count_operation,
                                     const TraceLevel level) {
  
  
  
  if ((level != kTraceAll) && (count_operation == kAddRefNoCreate)) {
    if (!(level & level_filter())) {
      return NULL;
    }
  }
  TraceImpl* impl =
    GetStaticInstance<TraceImpl>(count_operation);
  return impl;
}

TraceImpl* TraceImpl::GetTrace(const TraceLevel level) {
  return StaticInstance(kAddRefNoCreate, level);
}

TraceImpl* TraceImpl::CreateInstance() {
#if defined(_WIN32)
  return new TraceWindows();
#else
  return new TracePosix();
#endif
}

TraceImpl::TraceImpl()
    : critsect_interface_(CriticalSectionWrapper::CreateCriticalSection()),
      callback_(NULL),
      row_count_text_(0),
      file_count_text_(0),
      trace_file_(*FileWrapper::Create()),
      thread_(*ThreadWrapper::CreateThread(TraceImpl::Run, this,
                                           kHighestPriority, "Trace")),
      event_(*EventWrapper::Create()),
      critsect_array_(CriticalSectionWrapper::CreateCriticalSection()),
      next_free_idx_(),
      level_(),
      length_(),
      message_queue_(),
      active_queue_(0) {
  next_free_idx_[0] = 0;
  next_free_idx_[1] = 0;

  unsigned int tid = 0;
  thread_.Start(tid);

  for (int m = 0; m < WEBRTC_TRACE_NUM_ARRAY; ++m) {
    for (int n = 0; n < WEBRTC_TRACE_MAX_QUEUE; ++n) {
      message_queue_[m][n] = NULL;
    }
  }
#if !defined(WEBRTC_LAZY_TRACE_ALLOC)
  AllocateTraceBuffers();
#endif
}

bool TraceImpl::StopThread() {
  
  event_.Set();

  
  
  
  
  SleepMs(10);

  thread_.SetNotAlive();
  
  
  event_.Set();
  bool stopped = thread_.Stop();

  CriticalSectionScoped lock(critsect_interface_);
  trace_file_.Flush();
  trace_file_.CloseFile();
  return stopped;
}

TraceImpl::~TraceImpl() {
  StopThread();
  delete &event_;
  delete &trace_file_;
  delete &thread_;
  delete critsect_interface_;
  delete critsect_array_;

  for (int m = 0; m < WEBRTC_TRACE_NUM_ARRAY; ++m) {
    for (int n = 0; n < WEBRTC_TRACE_MAX_QUEUE; ++n) {
      delete [] message_queue_[m][n];
    }
  }
}

int32_t TraceImpl::AddThreadId(char* trace_message) const {
  uint32_t thread_id = ThreadWrapper::GetThreadId();
  
  return sprintf(trace_message, "%10u; ", thread_id);
}

int32_t TraceImpl::AddLevel(char* sz_message, const TraceLevel level) const {
  const int kMessageLength = 12;
  switch (level) {
    case kTraceTerseInfo:
      
      memset(sz_message, ' ', kMessageLength);
      sz_message[kMessageLength] = '\0';
      break;
    case kTraceStateInfo:
      sprintf(sz_message, "STATEINFO ; ");
      break;
    case kTraceWarning:
      sprintf(sz_message, "WARNING   ; ");
      break;
    case kTraceError:
      sprintf(sz_message, "ERROR     ; ");
      break;
    case kTraceCritical:
      sprintf(sz_message, "CRITICAL  ; ");
      break;
    case kTraceInfo:
      sprintf(sz_message, "DEBUGINFO ; ");
      break;
    case kTraceModuleCall:
      sprintf(sz_message, "MODULECALL; ");
      break;
    case kTraceMemory:
      sprintf(sz_message, "MEMORY    ; ");
      break;
    case kTraceTimer:
      sprintf(sz_message, "TIMER     ; ");
      break;
    case kTraceStream:
      sprintf(sz_message, "STREAM    ; ");
      break;
    case kTraceApiCall:
      sprintf(sz_message, "APICALL   ; ");
      break;
    case kTraceDebug:
      sprintf(sz_message, "DEBUG     ; ");
      break;
    default:
      assert(false);
      return 0;
  }
  
  return kMessageLength;
}

int32_t TraceImpl::AddModuleAndId(char* trace_message,
                                  const TraceModule module,
                                  const int32_t id) const {
  
  
  
  
  const long int idl = id;
  const int kMessageLength = 25;
  if (idl != -1) {
    const unsigned long int id_engine = id >> 16;
    const unsigned long int id_channel = id & 0xffff;

    switch (module) {
      case kTraceUndefined:
        
        memset(trace_message, ' ', kMessageLength);
        trace_message[kMessageLength] = '\0';
        break;
      case kTraceVoice:
        sprintf(trace_message, "       VOICE:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceVideo:
        sprintf(trace_message, "       VIDEO:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceUtility:
        sprintf(trace_message, "     UTILITY:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceRtpRtcp:
        sprintf(trace_message, "    RTP/RTCP:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceTransport:
        sprintf(trace_message, "   TRANSPORT:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceAudioCoding:
        sprintf(trace_message, "AUDIO CODING:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceSrtp:
        sprintf(trace_message, "        SRTP:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceAudioMixerServer:
        sprintf(trace_message, " AUDIO MIX/S:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceAudioMixerClient:
        sprintf(trace_message, " AUDIO MIX/C:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceVideoCoding:
        sprintf(trace_message, "VIDEO CODING:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceVideoMixer:
        
        sprintf(trace_message, "   VIDEO MIX:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceFile:
        sprintf(trace_message, "        FILE:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceAudioProcessing:
        sprintf(trace_message, "  AUDIO PROC:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceAudioDevice:
        sprintf(trace_message, "AUDIO DEVICE:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceVideoRenderer:
        sprintf(trace_message, "VIDEO RENDER:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceVideoCapture:
        sprintf(trace_message, "VIDEO CAPTUR:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceVideoPreocessing:
        sprintf(trace_message, "  VIDEO PROC:%5ld %5ld;", id_engine,
                id_channel);
        break;
      case kTraceRemoteBitrateEstimator:
        sprintf(trace_message, "     BWE RBE:%5ld %5ld;", id_engine,
                id_channel);
        break;
    }
  } else {
    switch (module) {
      case kTraceUndefined:
        
        memset(trace_message, ' ', kMessageLength);
        trace_message[kMessageLength] = '\0';
        break;
      case kTraceVoice:
        sprintf(trace_message, "       VOICE:%11ld;", idl);
        break;
      case kTraceVideo:
        sprintf(trace_message, "       VIDEO:%11ld;", idl);
        break;
      case kTraceUtility:
        sprintf(trace_message, "     UTILITY:%11ld;", idl);
        break;
      case kTraceRtpRtcp:
        sprintf(trace_message, "    RTP/RTCP:%11ld;", idl);
        break;
      case kTraceTransport:
        sprintf(trace_message, "   TRANSPORT:%11ld;", idl);
        break;
      case kTraceAudioCoding:
        sprintf(trace_message, "AUDIO CODING:%11ld;", idl);
        break;
      case kTraceSrtp:
        sprintf(trace_message, "        SRTP:%11ld;", idl);
        break;
      case kTraceAudioMixerServer:
        sprintf(trace_message, " AUDIO MIX/S:%11ld;", idl);
        break;
      case kTraceAudioMixerClient:
        sprintf(trace_message, " AUDIO MIX/C:%11ld;", idl);
        break;
      case kTraceVideoCoding:
        sprintf(trace_message, "VIDEO CODING:%11ld;", idl);
        break;
      case kTraceVideoMixer:
        sprintf(trace_message, "   VIDEO MIX:%11ld;", idl);
        break;
      case kTraceFile:
        sprintf(trace_message, "        FILE:%11ld;", idl);
        break;
      case kTraceAudioProcessing:
        sprintf(trace_message, "  AUDIO PROC:%11ld;", idl);
        break;
      case kTraceAudioDevice:
        sprintf(trace_message, "AUDIO DEVICE:%11ld;", idl);
        break;
      case kTraceVideoRenderer:
        sprintf(trace_message, "VIDEO RENDER:%11ld;", idl);
        break;
      case kTraceVideoCapture:
        sprintf(trace_message, "VIDEO CAPTUR:%11ld;", idl);
        break;
      case kTraceVideoPreocessing:
        sprintf(trace_message, "  VIDEO PROC:%11ld;", idl);
        break;
      case kTraceRemoteBitrateEstimator:
        sprintf(trace_message, "     BWE RBE:%11ld;", idl);
        break;
    }
  }
  return kMessageLength;
}

void TraceImpl::AllocateTraceBuffers()
{
  
  
  
  CriticalSectionScoped lock(critsect_array_);

  if (!message_queue_[0][0]) {
    for (int m = 0; m < WEBRTC_TRACE_NUM_ARRAY; ++m) {
      for (int n = 0; n < WEBRTC_TRACE_MAX_QUEUE; ++n) {
        message_queue_[m][n] = new char[WEBRTC_TRACE_MAX_MESSAGE_SIZE];
      }
    }
  }
}

int32_t TraceImpl::SetTraceFileImpl(const char* file_name_utf8,
                                    const bool add_file_counter) {
#if defined(WEBRTC_LAZY_TRACE_ALLOC)
  if (file_name_utf8) {
    AllocateTraceBuffers();
  }
#endif

  CriticalSectionScoped lock(critsect_interface_);

  trace_file_.Flush();
  trace_file_.CloseFile();

  if (file_name_utf8) {
    if (add_file_counter) {
      file_count_text_ = 1;

      char file_name_with_counter_utf8[FileWrapper::kMaxFileNameSize];
      CreateFileName(file_name_utf8, file_name_with_counter_utf8,
                     file_count_text_);
      if (trace_file_.OpenFile(file_name_with_counter_utf8, false, false,
                               true) == -1) {
        return -1;
      }
    } else {
      file_count_text_ = 0;
      if (trace_file_.OpenFile(file_name_utf8, false, false, true) == -1) {
        return -1;
      }
    }
  }
  row_count_text_ = 0;
  return 0;
}

int32_t TraceImpl::TraceFileImpl(
    char file_name_utf8[FileWrapper::kMaxFileNameSize]) {
  CriticalSectionScoped lock(critsect_interface_);
  return trace_file_.FileName(file_name_utf8, FileWrapper::kMaxFileNameSize);
}

int32_t TraceImpl::SetTraceCallbackImpl(TraceCallback* callback) {
#if defined(WEBRTC_LAZY_TRACE_ALLOC)
  if (callback) {
    AllocateTraceBuffers();
  }
#endif
  CriticalSectionScoped lock(critsect_interface_);
  callback_ = callback;
  return 0;
}

int32_t TraceImpl::AddMessage(
    char* trace_message,
    const char msg[WEBRTC_TRACE_MAX_MESSAGE_SIZE],
    const uint16_t written_so_far) const {
  int length = 0;
  if (written_so_far >= WEBRTC_TRACE_MAX_MESSAGE_SIZE) {
    return -1;
  }
  
#ifdef _WIN32
  length = _snprintf(trace_message,
                     WEBRTC_TRACE_MAX_MESSAGE_SIZE - written_so_far - 2,
                     "%s", msg);
  if (length < 0) {
    length = WEBRTC_TRACE_MAX_MESSAGE_SIZE - written_so_far - 2;
    trace_message[length] = 0;
  }
#else
  length = snprintf(trace_message,
                    WEBRTC_TRACE_MAX_MESSAGE_SIZE - written_so_far - 2,
                    "%s", msg);
  if (length < 0 ||
      length > WEBRTC_TRACE_MAX_MESSAGE_SIZE - written_so_far - 2) {
    length = WEBRTC_TRACE_MAX_MESSAGE_SIZE - written_so_far - 2;
    trace_message[length] = 0;
  }
#endif
  
  return length + 1;
}

void TraceImpl::AddMessageToList(
    const char trace_message[WEBRTC_TRACE_MAX_MESSAGE_SIZE],
    const uint16_t length,
    const TraceLevel level) {

#ifdef WEBRTC_DIRECT_TRACE
  if (callback_) {
    callback_->Print(level, trace_message, length);
  }
  return;
#endif

  CriticalSectionScoped lock(critsect_array_);

  uint16_t idx = next_free_idx_[active_queue_];

#if defined(WEBRTC_LAZY_TRACE_ALLOC)
  
  
  
  
  if (!message_queue_[active_queue_][idx]) {
  return;
}
#endif

  if (idx >= WEBRTC_TRACE_MAX_QUEUE) {
    if (!trace_file_.Open() && !callback_) {
      
      
      
      
      for (int n = 0; n < WEBRTC_TRACE_MAX_QUEUE / 4; ++n) {
        const int last_quarter_offset = (3 * WEBRTC_TRACE_MAX_QUEUE / 4);
        memcpy(message_queue_[active_queue_][n],
               message_queue_[active_queue_][n + last_quarter_offset],
               WEBRTC_TRACE_MAX_MESSAGE_SIZE);
      }
      idx = next_free_idx_[active_queue_] = WEBRTC_TRACE_MAX_QUEUE / 4;
    } else {
      
      
      
      
      
      
      
      return;
    }
  }

  next_free_idx_[active_queue_]++;

  level_[active_queue_][idx] = level;
  length_[active_queue_][idx] = length;
  memcpy(message_queue_[active_queue_][idx], trace_message, length);

  if (next_free_idx_[active_queue_] >= WEBRTC_TRACE_MAX_QUEUE - 1) {
    
    const char warning_msg[] = "WARNING MISSING TRACE MESSAGES\n";
    level_[active_queue_][WEBRTC_TRACE_MAX_QUEUE-1] = kTraceWarning;
    length_[active_queue_][WEBRTC_TRACE_MAX_QUEUE-1] = strlen(warning_msg);
    memcpy(message_queue_[active_queue_][WEBRTC_TRACE_MAX_QUEUE-1],
           warning_msg, length_[active_queue_][WEBRTC_TRACE_MAX_QUEUE-1]);
    next_free_idx_[active_queue_]++;
  }
}

bool TraceImpl::Run(void* obj) {
  return static_cast<TraceImpl*>(obj)->Process();
}

bool TraceImpl::Process() {
  if (event_.Wait(1000) == kEventSignaled) {
    
    
    critsect_interface_->Enter();
    bool write_to_file = trace_file_.Open() || callback_;
    critsect_interface_->Leave();
    if (write_to_file) {
      WriteToFile();
    }
  } else {
    CriticalSectionScoped lock(critsect_interface_);
    trace_file_.Flush();
  }
  return true;
}

void TraceImpl::WriteToFile() {
  uint8_t local_queue_active = 0;
  uint16_t local_next_free_idx = 0;

  
  
  
  {
    CriticalSectionScoped lock(critsect_array_);
    local_next_free_idx = next_free_idx_[active_queue_];
    next_free_idx_[active_queue_] = 0;
    local_queue_active = active_queue_;
    if (active_queue_ == 0) {
      active_queue_ = 1;
    } else {
      active_queue_ = 0;
    }
  }
  if (local_next_free_idx == 0) {
    return;
  }

  CriticalSectionScoped lock(critsect_interface_);

  for (uint16_t idx = 0; idx < local_next_free_idx; ++idx) {
    TraceLevel local_level = level_[local_queue_active][idx];
    if (callback_) {
      callback_->Print(local_level, message_queue_[local_queue_active][idx],
                       length_[local_queue_active][idx]);
    }
    if (trace_file_.Open()) {
      if (row_count_text_ > WEBRTC_TRACE_MAX_FILE_SIZE) {
        
        row_count_text_ = 0;
        trace_file_.Flush();

        if (file_count_text_ == 0) {
          trace_file_.Rewind();
        } else {
          char old_file_name[FileWrapper::kMaxFileNameSize];
          char new_file_name[FileWrapper::kMaxFileNameSize];

          
          trace_file_.FileName(old_file_name,
                               FileWrapper::kMaxFileNameSize);
          trace_file_.CloseFile();

          file_count_text_++;

          UpdateFileName(old_file_name, new_file_name, file_count_text_);

          if (trace_file_.OpenFile(new_file_name, false, false,
                                   true) == -1) {
            return;
          }
        }
      }
      if (row_count_text_ ==  0) {
        char message[WEBRTC_TRACE_MAX_MESSAGE_SIZE + 1];
        int32_t length = AddDateTimeInfo(message);
        if (length != -1) {
          message[length] = 0;
          message[length - 1] = '\n';
          trace_file_.Write(message, length);
          row_count_text_++;
        }
        length = AddBuildInfo(message);
        if (length != -1) {
          message[length + 1] = 0;
          message[length] = '\n';
          message[length - 1] = '\n';
          trace_file_.Write(message, length + 1);
          row_count_text_++;
          row_count_text_++;
        }
      }
      uint16_t length = length_[local_queue_active][idx];
      message_queue_[local_queue_active][idx][length] = 0;
      message_queue_[local_queue_active][idx][length - 1] = '\n';
      trace_file_.Write(message_queue_[local_queue_active][idx], length);
      row_count_text_++;
    }
  }
}

void TraceImpl::AddImpl(const TraceLevel level, const TraceModule module,
                        const int32_t id,
                        const char msg[WEBRTC_TRACE_MAX_MESSAGE_SIZE]) {
  if (TraceCheck(level)) {
    char trace_message[WEBRTC_TRACE_MAX_MESSAGE_SIZE];
    char* message_ptr = trace_message;

    int32_t len = 0;
    int32_t ack_len = 0;

    len = AddLevel(message_ptr, level);
    if (len == -1) {
      return;
    }
    message_ptr += len;
    ack_len += len;

    len = AddTime(message_ptr, level);
    if (len == -1) {
      return;
    }
    message_ptr += len;
    ack_len += len;

    len = AddModuleAndId(message_ptr, module, id);
    if (len == -1) {
      return;
    }
    message_ptr += len;
    ack_len += len;

    len = AddThreadId(message_ptr);
    if (len < 0) {
      return;
    }
    message_ptr += len;
    ack_len += len;

    len = AddMessage(message_ptr, msg, (uint16_t)ack_len);
    if (len == -1) {
      return;
    }
    ack_len += len;
    AddMessageToList(trace_message, (uint16_t)ack_len, level);

    
    event_.Set();
  }
}

bool TraceImpl::TraceCheck(const TraceLevel level) const {
  return (level & level_filter()) ? true : false;
}

bool TraceImpl::UpdateFileName(
    const char file_name_utf8[FileWrapper::kMaxFileNameSize],
    char file_name_with_counter_utf8[FileWrapper::kMaxFileNameSize],
    const uint32_t new_count) const {
  int32_t length = (int32_t)strlen(file_name_utf8);
  if (length < 0) {
    return false;
  }

  int32_t length_without_file_ending = length - 1;
  while (length_without_file_ending > 0) {
    if (file_name_utf8[length_without_file_ending] == '.') {
      break;
    } else {
      length_without_file_ending--;
    }
  }
  if (length_without_file_ending == 0) {
    length_without_file_ending = length;
  }
  int32_t length_to_ = length_without_file_ending - 1;
  while (length_to_ > 0) {
    if (file_name_utf8[length_to_] == '_') {
      break;
    } else {
      length_to_--;
    }
  }

  memcpy(file_name_with_counter_utf8, file_name_utf8, length_to_);
  sprintf(file_name_with_counter_utf8 + length_to_, "_%lu%s",
          static_cast<long unsigned int>(new_count),
          file_name_utf8 + length_without_file_ending);
  return true;
}

bool TraceImpl::CreateFileName(
    const char file_name_utf8[FileWrapper::kMaxFileNameSize],
    char file_name_with_counter_utf8[FileWrapper::kMaxFileNameSize],
    const uint32_t new_count) const {
  int32_t length = (int32_t)strlen(file_name_utf8);
  if (length < 0) {
    return false;
  }

  int32_t length_without_file_ending = length - 1;
  while (length_without_file_ending > 0) {
    if (file_name_utf8[length_without_file_ending] == '.') {
      break;
    } else {
      length_without_file_ending--;
    }
  }
  if (length_without_file_ending == 0) {
    length_without_file_ending = length;
  }
  memcpy(file_name_with_counter_utf8, file_name_utf8,
         length_without_file_ending);
  sprintf(file_name_with_counter_utf8 + length_without_file_ending, "_%lu%s",
          static_cast<long unsigned int>(new_count),
          file_name_utf8 + length_without_file_ending);
  return true;
}

void Trace::CreateTrace() {
  TraceImpl::StaticInstance(kAddRef);
}

void Trace::ReturnTrace() {
  TraceImpl::StaticInstance(kRelease);
}

int32_t Trace::TraceFile(char file_name[FileWrapper::kMaxFileNameSize]) {
  TraceImpl* trace = TraceImpl::GetTrace();
  if (trace) {
    int ret_val = trace->TraceFileImpl(file_name);
    ReturnTrace();
    return ret_val;
  }
  return -1;
}

int32_t Trace::SetTraceFile(const char* file_name,
                            const bool add_file_counter) {
  TraceImpl* trace = TraceImpl::GetTrace();
  if (trace) {
    int ret_val = trace->SetTraceFileImpl(file_name, add_file_counter);
    ReturnTrace();
    return ret_val;
  }
  return -1;
}

int32_t Trace::SetTraceCallback(TraceCallback* callback) {
  TraceImpl* trace = TraceImpl::GetTrace();
  if (trace) {
    int ret_val = trace->SetTraceCallbackImpl(callback);
    ReturnTrace();
    return ret_val;
  }
  return -1;
}

void Trace::Add(const TraceLevel level, const TraceModule module,
                const int32_t id, const char* msg, ...) {
  TraceImpl* trace = TraceImpl::GetTrace(level);
  if (trace) {
    if (trace->TraceCheck(level)) {
      char temp_buff[WEBRTC_TRACE_MAX_MESSAGE_SIZE];
      char* buff = 0;
      if (msg) {
        va_list args;
        va_start(args, msg);
#ifdef _WIN32
        _vsnprintf(temp_buff, WEBRTC_TRACE_MAX_MESSAGE_SIZE - 1, msg, args);
#else
        vsnprintf(temp_buff, WEBRTC_TRACE_MAX_MESSAGE_SIZE - 1, msg, args);
#endif
        va_end(args);
        buff = temp_buff;
      }
      trace->AddImpl(level, module, id, buff);
    }
    ReturnTrace();
  }
}

}  
