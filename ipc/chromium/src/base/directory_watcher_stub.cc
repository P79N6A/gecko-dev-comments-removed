






#include "base/directory_watcher.h"

class DirectoryWatcherImpl : public DirectoryWatcher::PlatformDelegate {
 public:
  virtual bool Watch(const FilePath& path, DirectoryWatcher::Delegate* delegate,
                     bool recursive) {
    return false;
  }
};

DirectoryWatcher::DirectoryWatcher() {
  impl_ = new DirectoryWatcherImpl();
}
