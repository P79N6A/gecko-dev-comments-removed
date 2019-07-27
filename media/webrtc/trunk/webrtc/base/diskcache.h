









#ifndef WEBRTC_BASE_DISKCACHE_H__
#define WEBRTC_BASE_DISKCACHE_H__

#include <map>
#include <string>

#if defined(WEBRTC_WIN)
#undef UnlockResource
#endif  

namespace rtc {

class StreamInterface;













class DiskCache {
public:
  DiskCache();
  virtual ~DiskCache();

  bool Initialize(const std::string& folder, size_t size);
  bool Purge();

  bool LockResource(const std::string& id);
  StreamInterface* WriteResource(const std::string& id, size_t index);
  bool UnlockResource(const std::string& id);

  StreamInterface* ReadResource(const std::string& id, size_t index) const;

  bool HasResource(const std::string& id) const;
  bool HasResourceStream(const std::string& id, size_t index) const;
  bool DeleteResource(const std::string& id);

 protected:
  virtual bool InitializeEntries() = 0;
  virtual bool PurgeFiles() = 0;

  virtual bool FileExists(const std::string& filename) const = 0;
  virtual bool DeleteFile(const std::string& filename) const = 0;

  enum LockState { LS_UNLOCKED, LS_LOCKED, LS_UNLOCKING };
  struct Entry {
    LockState lock_state;
    mutable size_t accessors;
    size_t size;
    size_t streams;
    time_t last_modified;
  };
  typedef std::map<std::string, Entry> EntryMap;
  friend class DiskCacheAdapter;

  bool CheckLimit();

  std::string IdToFilename(const std::string& id, size_t index) const;
  bool FilenameToId(const std::string& filename, std::string* id,
                    size_t* index) const;

  const Entry* GetEntry(const std::string& id) const {
    return const_cast<DiskCache*>(this)->GetOrCreateEntry(id, false);
  }
  Entry* GetOrCreateEntry(const std::string& id, bool create);

  void ReleaseResource(const std::string& id, size_t index) const;

  std::string folder_;
  size_t max_cache_, total_size_;
  EntryMap map_;
  mutable size_t total_accessors_;
};






class CacheLock {
public:
  CacheLock(DiskCache* cache, const std::string& id, bool rollback = false)
  : cache_(cache), id_(id), rollback_(rollback)
  {
    locked_ = cache_->LockResource(id_);
  }
  ~CacheLock() {
    if (locked_) {
      cache_->UnlockResource(id_);
      if (rollback_) {
        cache_->DeleteResource(id_);
      }
    }
  }
  bool IsLocked() const { return locked_; }
  void Commit() { rollback_ = false; }

private:
  DiskCache* cache_;
  std::string id_;
  bool rollback_, locked_;
};



}  

#endif 
