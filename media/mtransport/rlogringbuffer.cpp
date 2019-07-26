








#include <cstdarg>

#include "rlogringbuffer.h"

#include <deque>
#include <string>
#include "mozilla/Assertions.h"
#include "mozilla/Move.h" 
#include "mozilla/Mutex.h"
#include "mozilla/NullPtr.h"
#include <vector>

extern "C" {
#include <csi_platform.h>
#include "r_log.h"
}


static int ringbuffer_vlog(int facility,
                           int level,
                           const char *format,
                           va_list ap) {
  MOZ_ASSERT(mozilla::RLogRingBuffer::GetInstance());
  
  
  char temp[4096];
  vsnprintf(temp, sizeof(temp), format, ap);

  mozilla::RLogRingBuffer::GetInstance()->Log(std::string(temp));
  return 0;
}

namespace mozilla {

RLogRingBuffer* RLogRingBuffer::instance;

RLogRingBuffer::RLogRingBuffer()
  : log_limit_(4096),
    mutex_("RLogRingBuffer::mutex_") {
}

RLogRingBuffer::~RLogRingBuffer() {
}

void RLogRingBuffer::SetLogLimit(uint32_t new_limit) {
  OffTheBooksMutexAutoLock lock(mutex_);
  log_limit_ = new_limit;
  RemoveOld();
}

void RLogRingBuffer::Log(std::string&& log) {
  OffTheBooksMutexAutoLock lock(mutex_);
  log_messages_.push_front(Move(log));
  RemoveOld();
}

inline void RLogRingBuffer::RemoveOld() {
  if (log_messages_.size() > log_limit_) {
    log_messages_.resize(log_limit_);
  }
}


RLogRingBuffer* RLogRingBuffer::CreateInstance() {
  if (!instance) {
    instance = new RLogRingBuffer;
    r_log_set_extra_destination(LOG_INFO, &ringbuffer_vlog);
  }
  return instance;
}

RLogRingBuffer* RLogRingBuffer::GetInstance() {
  return instance;
}

void RLogRingBuffer::DestroyInstance() {
  
  r_log_set_extra_destination(LOG_INFO, nullptr);
  delete instance;
  instance = nullptr;
}

void RLogRingBuffer::Filter(const std::string& substring,
                            uint32_t limit,
                            std::deque<std::string>* matching_logs) {
  std::vector<std::string> substrings;
  substrings.push_back(substring);
  FilterAny(substrings, limit, matching_logs);
}

inline bool AnySubstringMatches(const std::vector<std::string>& substrings,
                                const std::string& string) {
  for (auto sub = substrings.begin(); sub != substrings.end(); ++sub) {
    if (string.find(*sub) != std::string::npos) {
      return true;
    }
  }
  return false;
}

void RLogRingBuffer::FilterAny(const std::vector<std::string>& substrings,
                               uint32_t limit,
                               std::deque<std::string>* matching_logs) {
  OffTheBooksMutexAutoLock lock(mutex_);
  if (limit == 0) {
    
    limit = log_limit_;
  }

  for (auto log = log_messages_.begin();
       log != log_messages_.end() && matching_logs->size() < limit;
       ++log) {
    if (AnySubstringMatches(substrings, *log)) {
      matching_logs->push_front(*log);
    }
  }
}

} 

