































#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <stdio.h>
#include <errno.h>
#include <vector>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define snprintf _snprintf    // see comment in strutil.cc
#elif defined(HAVE_PTHREAD_H)
#include <pthread.h>
#else
#error "No suitable threading library available."
#endif

namespace google {
namespace protobuf {

namespace internal {

void VerifyVersion(int headerVersion,
                   int minLibraryVersion,
                   const char* filename) {
  if (GOOGLE_PROTOBUF_VERSION < minLibraryVersion) {
    
    GOOGLE_LOG(FATAL)
      << "This program requires version " << VersionString(minLibraryVersion)
      << " of the Protocol Buffer runtime library, but the installed version "
         "is " << VersionString(GOOGLE_PROTOBUF_VERSION) << ".  Please update "
         "your library.  If you compiled the program yourself, make sure that "
         "your headers are from the same version of Protocol Buffers as your "
         "link-time library.  (Version verification failed in \""
      << filename << "\".)";
  }
  if (headerVersion < kMinHeaderVersionForLibrary) {
    
    GOOGLE_LOG(FATAL)
      << "This program was compiled against version "
      << VersionString(headerVersion) << " of the Protocol Buffer runtime "
         "library, which is not compatible with the installed version ("
      << VersionString(GOOGLE_PROTOBUF_VERSION) <<  ").  Contact the program "
         "author for an update.  If you compiled the program yourself, make "
         "sure that your headers are from the same version of Protocol Buffers "
         "as your link-time library.  (Version verification failed in \""
      << filename << "\".)";
  }
}

string VersionString(int version) {
  int major = version / 1000000;
  int minor = (version / 1000) % 1000;
  int micro = version % 1000;

  
  
  char buffer[128];
  snprintf(buffer, sizeof(buffer), "%d.%d.%d", major, minor, micro);

  
  buffer[sizeof(buffer)-1] = '\0';

  return buffer;
}

}  




namespace internal {

void DefaultLogHandler(LogLevel level, const char* filename, int line,
                       const string& message) {
  static const char* level_names[] = { "INFO", "WARNING", "ERROR", "FATAL" };

  
  
  fprintf(stderr, "[libprotobuf %s %s:%d] %s\n",
          level_names[level], filename, line, message.c_str());
  fflush(stderr);  
}

void NullLogHandler(LogLevel , const char* ,
                    int , const string& ) {
  
}

static LogHandler* log_handler_ = &DefaultLogHandler;
static int log_silencer_count_ = 0;

static Mutex* log_silencer_count_mutex_ = NULL;
GOOGLE_PROTOBUF_DECLARE_ONCE(log_silencer_count_init_);

void DeleteLogSilencerCount() {
  delete log_silencer_count_mutex_;
  log_silencer_count_mutex_ = NULL;
}
void InitLogSilencerCount() {
  log_silencer_count_mutex_ = new Mutex;
  OnShutdown(&DeleteLogSilencerCount);
}
void InitLogSilencerCountOnce() {
  GoogleOnceInit(&log_silencer_count_init_, &InitLogSilencerCount);
}

LogMessage& LogMessage::operator<<(const string& value) {
  message_ += value;
  return *this;
}

LogMessage& LogMessage::operator<<(const char* value) {
  message_ += value;
  return *this;
}




#undef DECLARE_STREAM_OPERATOR
#define DECLARE_STREAM_OPERATOR(TYPE, FORMAT)                       \
  LogMessage& LogMessage::operator<<(TYPE value) {                  \
    /* 128 bytes should be big enough for any of the primitive */   \
    /* values which we print with this, but well use snprintf() */  \
    /* anyway to be extra safe. */                                  \
    char buffer[128];                                               \
    snprintf(buffer, sizeof(buffer), FORMAT, value);                \
    /* Guard against broken MSVC snprintf(). */                     \
    buffer[sizeof(buffer)-1] = '\0';                                \
    message_ += buffer;                                             \
    return *this;                                                   \
  }

DECLARE_STREAM_OPERATOR(char         , "%c" )
DECLARE_STREAM_OPERATOR(int          , "%d" )
DECLARE_STREAM_OPERATOR(uint         , "%u" )
DECLARE_STREAM_OPERATOR(long         , "%ld")
DECLARE_STREAM_OPERATOR(unsigned long, "%lu")
DECLARE_STREAM_OPERATOR(double       , "%g" )
#undef DECLARE_STREAM_OPERATOR

LogMessage::LogMessage(LogLevel level, const char* filename, int line)
  : level_(level), filename_(filename), line_(line) {}
LogMessage::~LogMessage() {}

void LogMessage::Finish() {
  bool suppress = false;

  if (level_ != LOGLEVEL_FATAL) {
    InitLogSilencerCountOnce();
    MutexLock lock(log_silencer_count_mutex_);
    suppress = log_silencer_count_ > 0;
  }

  if (!suppress) {
    log_handler_(level_, filename_, line_, message_);
  }

  if (level_ == LOGLEVEL_FATAL) {
#if PROTOBUF_USE_EXCEPTIONS
    throw FatalException(filename_, line_, message_);
#else
    abort();
#endif
  }
}

void LogFinisher::operator=(LogMessage& other) {
  other.Finish();
}

}  

LogHandler* SetLogHandler(LogHandler* new_func) {
  LogHandler* old = internal::log_handler_;
  if (old == &internal::NullLogHandler) {
    old = NULL;
  }
  if (new_func == NULL) {
    internal::log_handler_ = &internal::NullLogHandler;
  } else {
    internal::log_handler_ = new_func;
  }
  return old;
}

LogSilencer::LogSilencer() {
  internal::InitLogSilencerCountOnce();
  MutexLock lock(internal::log_silencer_count_mutex_);
  ++internal::log_silencer_count_;
};

LogSilencer::~LogSilencer() {
  internal::InitLogSilencerCountOnce();
  MutexLock lock(internal::log_silencer_count_mutex_);
  --internal::log_silencer_count_;
};




Closure::~Closure() {}

namespace internal { FunctionClosure0::~FunctionClosure0() {} }

void DoNothing() {}




#ifdef _WIN32

struct Mutex::Internal {
  CRITICAL_SECTION mutex;
#ifndef NDEBUG
  
  DWORD thread_id;
#endif
};

Mutex::Mutex()
  : mInternal(new Internal) {
  InitializeCriticalSection(&mInternal->mutex);
}

Mutex::~Mutex() {
  DeleteCriticalSection(&mInternal->mutex);
  delete mInternal;
}

void Mutex::Lock() {
  EnterCriticalSection(&mInternal->mutex);
#ifndef NDEBUG
  mInternal->thread_id = GetCurrentThreadId();
#endif
}

void Mutex::Unlock() {
#ifndef NDEBUG
  mInternal->thread_id = 0;
#endif
  LeaveCriticalSection(&mInternal->mutex);
}

void Mutex::AssertHeld() {
#ifndef NDEBUG
  GOOGLE_DCHECK_EQ(mInternal->thread_id, GetCurrentThreadId());
#endif
}

#elif defined(HAVE_PTHREAD)

struct Mutex::Internal {
  pthread_mutex_t mutex;
};

Mutex::Mutex()
  : mInternal(new Internal) {
  pthread_mutex_init(&mInternal->mutex, NULL);
}

Mutex::~Mutex() {
  pthread_mutex_destroy(&mInternal->mutex);
  delete mInternal;
}

void Mutex::Lock() {
  int result = pthread_mutex_lock(&mInternal->mutex);
  if (result != 0) {
    GOOGLE_LOG(FATAL) << "pthread_mutex_lock: " << strerror(result);
  }
}

void Mutex::Unlock() {
  int result = pthread_mutex_unlock(&mInternal->mutex);
  if (result != 0) {
    GOOGLE_LOG(FATAL) << "pthread_mutex_unlock: " << strerror(result);
  }
}

void Mutex::AssertHeld() {
  
  
}

#endif







uint32 ghtonl(uint32 x) {
  union {
    uint32 result;
    uint8 result_array[4];
  };
  result_array[0] = static_cast<uint8>(x >> 24);
  result_array[1] = static_cast<uint8>((x >> 16) & 0xFF);
  result_array[2] = static_cast<uint8>((x >> 8) & 0xFF);
  result_array[3] = static_cast<uint8>(x & 0xFF);
  return result;
}




namespace internal {

typedef void OnShutdownFunc();
vector<void (*)()>* shutdown_functions = NULL;
Mutex* shutdown_functions_mutex = NULL;
GOOGLE_PROTOBUF_DECLARE_ONCE(shutdown_functions_init);

void InitShutdownFunctions() {
  shutdown_functions = new vector<void (*)()>;
  shutdown_functions_mutex = new Mutex;
}

inline void InitShutdownFunctionsOnce() {
  GoogleOnceInit(&shutdown_functions_init, &InitShutdownFunctions);
}

void OnShutdown(void (*func)()) {
  InitShutdownFunctionsOnce();
  MutexLock lock(shutdown_functions_mutex);
  shutdown_functions->push_back(func);
}

}  

void ShutdownProtobufLibrary() {
  internal::InitShutdownFunctionsOnce();

  
  
  

  
  if (internal::shutdown_functions == NULL) return;

  for (int i = 0; i < internal::shutdown_functions->size(); i++) {
    internal::shutdown_functions->at(i)();
  }
  delete internal::shutdown_functions;
  internal::shutdown_functions = NULL;
  delete internal::shutdown_functions_mutex;
  internal::shutdown_functions_mutex = NULL;
}

#if PROTOBUF_USE_EXCEPTIONS
FatalException::~FatalException() throw() {}

const char* FatalException::what() const throw() {
  return message_.c_str();
}
#endif

}  
}  
