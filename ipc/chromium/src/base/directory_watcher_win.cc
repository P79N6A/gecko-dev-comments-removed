



#include "base/directory_watcher.h"

#include "base/file_path.h"
#include "base/logging.h"
#include "base/object_watcher.h"
#include "base/ref_counted.h"

namespace {

class DirectoryWatcherImpl : public DirectoryWatcher::PlatformDelegate,
                             public base::ObjectWatcher::Delegate {
 public:
  DirectoryWatcherImpl() : handle_(INVALID_HANDLE_VALUE) {}
  virtual ~DirectoryWatcherImpl();

  virtual bool Watch(const FilePath& path, DirectoryWatcher::Delegate* delegate,
                     bool recursive);

  
  virtual void OnObjectSignaled(HANDLE object);

 private:
  
  DirectoryWatcher::Delegate* delegate_;
  
  FilePath path_;
  
  HANDLE handle_;
  
  base::ObjectWatcher watcher_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryWatcherImpl);
};

DirectoryWatcherImpl::~DirectoryWatcherImpl() {
  if (handle_ != INVALID_HANDLE_VALUE) {
    watcher_.StopWatching();
    FindCloseChangeNotification(handle_);
  }
}

bool DirectoryWatcherImpl::Watch(const FilePath& path,
    DirectoryWatcher::Delegate* delegate, bool recursive) {
  DCHECK(path_.value().empty());  

  handle_ = FindFirstChangeNotification(
      path.value().c_str(),
      recursive,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
      FILE_NOTIFY_CHANGE_LAST_WRITE);
  if (handle_ == INVALID_HANDLE_VALUE)
    return false;

  delegate_ = delegate;
  path_ = path;
  watcher_.StartWatching(handle_, this);

  return true;
}

void DirectoryWatcherImpl::OnObjectSignaled(HANDLE object) {
  DCHECK(object == handle_);
  
  scoped_refptr<DirectoryWatcherImpl> keep_alive(this);

  delegate_->OnDirectoryChanged(path_);

  
  BOOL ok = FindNextChangeNotification(object);
  DCHECK(ok);
  watcher_.StartWatching(object, this);
}

}  

DirectoryWatcher::DirectoryWatcher() {
  impl_ = new DirectoryWatcherImpl();
}
