









#ifndef WEBRTC_VIDEO_ENGINE_VIE_MANAGER_BASE_H_
#define WEBRTC_VIDEO_ENGINE_VIE_MANAGER_BASE_H_

namespace webrtc {

class RWLockWrapper;

class ViEManagerBase {
  friend class ViEManagedItemScopedBase;
  friend class ViEManagerScopedBase;
  friend class ViEManagerWriteScoped;
 public:
  ViEManagerBase();
  ~ViEManagerBase();

 private:
  
  void WriteLockManager();

  
  void ReleaseWriteLockManager();

  
  void ReadLockManager() const;

  
  void ReleaseLockManager() const;

  RWLockWrapper& instance_rwlock_;
};

class ViEManagerWriteScoped {
 public:
  explicit ViEManagerWriteScoped(ViEManagerBase* vie_manager);
  ~ViEManagerWriteScoped();

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
