









#ifndef WEBRTC_VIDEO_ENGINE_VIE_MANAGER_BASE_H_
#define WEBRTC_VIDEO_ENGINE_VIE_MANAGER_BASE_H_

#include "webrtc/system_wrappers/interface/thread_annotations.h"

namespace webrtc {

class RWLockWrapper;

class LOCKABLE ViEManagerBase {
  friend class ViEManagedItemScopedBase;
  friend class ViEManagerScopedBase;
  friend class ViEManagerWriteScoped;
 public:
  ViEManagerBase();
  ~ViEManagerBase();

 private:
  
  void WriteLockManager() EXCLUSIVE_LOCK_FUNCTION();

  
  void ReleaseWriteLockManager() UNLOCK_FUNCTION();

  
  void ReadLockManager() const SHARED_LOCK_FUNCTION();

  
  void ReleaseLockManager() const UNLOCK_FUNCTION();

  RWLockWrapper& instance_rwlock_;
};

class SCOPED_LOCKABLE ViEManagerWriteScoped {
 public:
  explicit ViEManagerWriteScoped(ViEManagerBase* vie_manager)
      EXCLUSIVE_LOCK_FUNCTION(vie_manager);
  ~ViEManagerWriteScoped() UNLOCK_FUNCTION();

 private:
  ViEManagerBase* vie_manager_;
};

class ViEManagerScopedBase {
  friend class ViEManagedItemScopedBase;
 public:
  explicit ViEManagerScopedBase(const ViEManagerBase& vie_manager);
  ~ViEManagerScopedBase();

 protected:
  const ViEManagerBase* vie_manager_;

 private:
  int ref_count_;
};

class ViEManagedItemScopedBase {
 public:
  explicit ViEManagedItemScopedBase(ViEManagerScopedBase* vie_scoped_manager);
  ~ViEManagedItemScopedBase();

 protected:
  ViEManagerScopedBase* vie_scoped_manager_;
};

}  

#endif  
