



#include "base/win/scoped_handle.h"

#include <unordered_map>

#include "base/debug/alias.h"
#include "base/hash.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/synchronization/lock_impl.h"

namespace {

struct HandleHash {
  size_t operator()(const HANDLE& handle) const {
    char buffer[sizeof(handle)];
    memcpy(buffer, &handle, sizeof(handle));
    return base::Hash(buffer, sizeof(buffer));
  }
};

struct Info {
  const void* owner;
  const void* pc1;
  const void* pc2;
  DWORD thread_id;
};
typedef std::unordered_map<HANDLE, Info, HandleHash> HandleMap;


typedef base::internal::LockImpl NativeLock;
base::LazyInstance<NativeLock>::Leaky g_lock = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<HandleMap>::Leaky g_handle_map = LAZY_INSTANCE_INITIALIZER;
bool g_closing = false;








bool g_verifier_enabled = true;

bool CloseHandleWrapper(HANDLE handle) {
  if (!::CloseHandle(handle))
    CHECK(false);
  return true;
}



class AutoNativeLock {
 public:
  explicit AutoNativeLock(NativeLock& lock) : lock_(lock) {
    lock_.Lock();
  }

  ~AutoNativeLock() {
    lock_.Unlock();
  }

 private:
  NativeLock& lock_;
  DISALLOW_COPY_AND_ASSIGN(AutoNativeLock);
};

}  

namespace base {
namespace win {


bool HandleTraits::CloseHandle(HANDLE handle) {
  if (!g_verifier_enabled)
    return CloseHandleWrapper(handle);

  AutoNativeLock lock(g_lock.Get());
  g_closing = true;
  CloseHandleWrapper(handle);
  g_closing = false;

  return true;
}


void VerifierTraits::StartTracking(HANDLE handle, const void* owner,
                                   const void* pc1, const void* pc2) {
  if (!g_verifier_enabled)
    return;

  
  DWORD thread_id = GetCurrentThreadId();

  AutoNativeLock lock(g_lock.Get());

  Info handle_info = { owner, pc1, pc2, thread_id };
  std::pair<HANDLE, Info> item(handle, handle_info);
  std::pair<HandleMap::iterator, bool> result = g_handle_map.Get().insert(item);
  if (!result.second) {
    Info other = result.first->second;
    debug::Alias(&other);
    CHECK(false);
  }
}


void VerifierTraits::StopTracking(HANDLE handle, const void* owner,
                                  const void* pc1, const void* pc2) {
  if (!g_verifier_enabled)
    return;

  AutoNativeLock lock(g_lock.Get());
  HandleMap::iterator i = g_handle_map.Get().find(handle);
  if (i == g_handle_map.Get().end())
    CHECK(false);

  Info other = i->second;
  if (other.owner != owner) {
    debug::Alias(&other);
    CHECK(false);
  }

  g_handle_map.Get().erase(i);
}

void DisableHandleVerifier() {
  g_verifier_enabled = false;
}

void OnHandleBeingClosed(HANDLE handle) {
  AutoNativeLock lock(g_lock.Get());
  if (g_closing)
    return;

  HandleMap::iterator i = g_handle_map.Get().find(handle);
  if (i == g_handle_map.Get().end())
    return;

  Info other = i->second;
  debug::Alias(&other);
  CHECK(false);
}

}  
}  
