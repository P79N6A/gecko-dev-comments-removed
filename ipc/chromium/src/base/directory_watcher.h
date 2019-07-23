





#ifndef BASE_DIRECTORY_WATCHER_H_
#define BASE_DIRECTORY_WATCHER_H_

#include "base/basictypes.h"
#include "base/ref_counted.h"

class FilePath;




class DirectoryWatcher {
 public:
  class Delegate {
   public:
    virtual void OnDirectoryChanged(const FilePath& path) = 0;
  };

  DirectoryWatcher();
  ~DirectoryWatcher() {}

  
  
  
  
  
  
  
  
  
  bool Watch(const FilePath& path, Delegate* delegate, bool recursive) {
    return impl_->Watch(path, delegate, recursive);
  }

  
  class PlatformDelegate : public base::RefCounted<PlatformDelegate> {
   public:
    virtual ~PlatformDelegate() {}
    virtual bool Watch(const FilePath& path, Delegate* delegate,
                       bool recursive) = 0;
  };

 private:
  scoped_refptr<PlatformDelegate> impl_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryWatcher);
};

#endif  
