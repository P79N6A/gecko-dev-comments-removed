



#ifndef BASE_THREADING_THREAD_ID_NAME_MANAGER_H_
#define BASE_THREADING_THREAD_ID_NAME_MANAGER_H_

#include <map>
#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"

template <typename T> struct DefaultSingletonTraits;

namespace base {

class BASE_EXPORT ThreadIdNameManager {
 public:
  static ThreadIdNameManager* GetInstance();

  static const char* GetDefaultInternedString();

  
  void RegisterThread(PlatformThreadHandle::Handle handle, PlatformThreadId id);

  
  void SetName(PlatformThreadId id, const char* name);

  
  const char* GetName(PlatformThreadId id);

  
  void RemoveName(PlatformThreadHandle::Handle handle, PlatformThreadId id);

 private:
  friend struct DefaultSingletonTraits<ThreadIdNameManager>;

  typedef std::map<PlatformThreadId, PlatformThreadHandle::Handle>
      ThreadIdToHandleMap;
  typedef std::map<PlatformThreadHandle::Handle, std::string*>
      ThreadHandleToInternedNameMap;
  typedef std::map<std::string, std::string*> NameToInternedNameMap;

  ThreadIdNameManager();
  ~ThreadIdNameManager();

  
  
  Lock lock_;

  NameToInternedNameMap name_to_interned_name_;
  ThreadIdToHandleMap thread_id_to_handle_;
  ThreadHandleToInternedNameMap thread_handle_to_interned_name_;

  
  std::string* main_process_name_;
  PlatformThreadId main_process_id_;

  DISALLOW_COPY_AND_ASSIGN(ThreadIdNameManager);
};

}  

#endif  
