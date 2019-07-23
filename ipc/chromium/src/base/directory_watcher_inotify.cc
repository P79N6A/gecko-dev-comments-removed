



#include "base/directory_watcher.h"

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

#include "base/eintr_wrapper.h"
#include "base/file_path.h"
#include "base/hash_tables.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/scoped_ptr.h"
#include "base/singleton.h"
#include "base/task.h"
#include "base/thread.h"

namespace {


class InotifyReader {
 public:
  typedef int Watch;  
  static const Watch kInvalidWatch = -1;

  
  
  
  
  
  Watch AddWatch(const FilePath& path, DirectoryWatcher::Delegate* delegate);

  
  
  
  bool RemoveWatch(Watch watch, DirectoryWatcher::Delegate* delegate);

  
  void OnInotifyEvent(inotify_event* event);

 private:
  friend struct DefaultSingletonTraits<InotifyReader>;

  typedef std::pair<DirectoryWatcher::Delegate*, MessageLoop*> DelegateInfo;
  typedef std::multiset<DelegateInfo> DelegateSet;

  InotifyReader();
  ~InotifyReader();

  
  
  
  base::hash_map<Watch, DelegateSet> delegates_;

  
  base::hash_map<Watch, FilePath> paths_;

  
  Lock lock_;

  
  base::Thread thread_;

  
  const int inotify_fd_;

  
  int shutdown_pipe_[2];

  
  bool valid_;

  DISALLOW_COPY_AND_ASSIGN(InotifyReader);
};

class InotifyReaderTask : public Task {
 public:
  InotifyReaderTask(InotifyReader* reader, int inotify_fd, int shutdown_fd)
      : reader_(reader),
        inotify_fd_(inotify_fd),
        shutdown_fd_(shutdown_fd) {
  }

  virtual void Run() {
    while (true) {
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(inotify_fd_, &rfds);
      FD_SET(shutdown_fd_, &rfds);

      
      int select_result =
        HANDLE_EINTR(select(std::max(inotify_fd_, shutdown_fd_) + 1,
                            &rfds, NULL, NULL, NULL));
      if (select_result < 0) {
        DLOG(WARNING) << "select failed: " << strerror(errno);
        return;
      }

      if (FD_ISSET(shutdown_fd_, &rfds))
        return;

      
      int buffer_size;
      int ioctl_result = HANDLE_EINTR(ioctl(inotify_fd_, FIONREAD,
                                            &buffer_size));

      if (ioctl_result != 0) {
        DLOG(WARNING) << "ioctl failed: " << strerror(errno);
        return;
      }

      std::vector<char> buffer(buffer_size);

      ssize_t bytes_read = HANDLE_EINTR(read(inotify_fd_, &buffer[0],
                                             buffer_size));

      if (bytes_read < 0) {
        DLOG(WARNING) << "read from inotify fd failed: " << strerror(errno);
        return;
      }

      ssize_t i = 0;
      while (i < bytes_read) {
        inotify_event* event = reinterpret_cast<inotify_event*>(&buffer[i]);
        size_t event_size = sizeof(inotify_event) + event->len;
        DCHECK(i + event_size <= static_cast<size_t>(bytes_read));
        reader_->OnInotifyEvent(event);
        i += event_size;
      }
    }
  }

 private:
  InotifyReader* reader_;
  int inotify_fd_;
  int shutdown_fd_;

  DISALLOW_COPY_AND_ASSIGN(InotifyReaderTask);
};

class InotifyReaderNotifyTask : public Task {
 public:
  InotifyReaderNotifyTask(DirectoryWatcher::Delegate* delegate,
                          const FilePath& path)
      : delegate_(delegate),
        path_(path) {
  }

  virtual void Run() {
    delegate_->OnDirectoryChanged(path_);
  }

 private:
  DirectoryWatcher::Delegate* delegate_;
  FilePath path_;

  DISALLOW_COPY_AND_ASSIGN(InotifyReaderNotifyTask);
};

InotifyReader::InotifyReader()
    : thread_("inotify_reader"),
      inotify_fd_(inotify_init()),
      valid_(false) {
  shutdown_pipe_[0] = -1;
  shutdown_pipe_[1] = -1;
  if (inotify_fd_ >= 0 && pipe(shutdown_pipe_) == 0 && thread_.Start()) {
    thread_.message_loop()->PostTask(
        FROM_HERE, new InotifyReaderTask(this, inotify_fd_, shutdown_pipe_[0]));
    valid_ = true;
  }
}

InotifyReader::~InotifyReader() {
  if (valid_) {
    
    
    HANDLE_EINTR(write(shutdown_pipe_[1], "", 1));
    thread_.Stop();
  }
  if (inotify_fd_ >= 0)
    close(inotify_fd_);
  if (shutdown_pipe_[0] >= 0)
    close(shutdown_pipe_[0]);
  if (shutdown_pipe_[1] >= 0)
    close(shutdown_pipe_[1]);
}

InotifyReader::Watch InotifyReader::AddWatch(
    const FilePath& path, DirectoryWatcher::Delegate* delegate) {
  if (!valid_)
    return kInvalidWatch;

  AutoLock auto_lock(lock_);

  Watch watch = inotify_add_watch(inotify_fd_, path.value().c_str(),
                                  IN_CREATE | IN_DELETE |
                                  IN_CLOSE_WRITE | IN_MOVE);
  if (watch == kInvalidWatch)
    return kInvalidWatch;

  if (paths_[watch].empty())
    paths_[watch] = path;  

  delegates_[watch].insert(std::make_pair(delegate, MessageLoop::current()));

  return watch;
}

bool InotifyReader::RemoveWatch(Watch watch,
                                DirectoryWatcher::Delegate* delegate) {
  if (!valid_)
    return false;

  AutoLock auto_lock(lock_);

  if (paths_[watch].empty())
    return false;  

  
  delegates_[watch].erase(
      delegates_[watch].find(std::make_pair(delegate, MessageLoop::current())));

  if (delegates_[watch].empty()) {
    paths_.erase(watch);
    delegates_.erase(watch);

    return (inotify_rm_watch(inotify_fd_, watch) == 0);
  }

  return true;
}

void InotifyReader::OnInotifyEvent(inotify_event* event) {
  if (event->mask & IN_IGNORED)
    return;

  DelegateSet delegates_to_notify;
  FilePath changed_path;

  {
    AutoLock auto_lock(lock_);
    changed_path = paths_[event->wd];
    delegates_to_notify.insert(delegates_[event->wd].begin(),
                               delegates_[event->wd].end());
  }

  DelegateSet::iterator i;
  for (i = delegates_to_notify.begin(); i != delegates_to_notify.end(); ++i) {
    DirectoryWatcher::Delegate* delegate = i->first;
    MessageLoop* loop = i->second;
    loop->PostTask(FROM_HERE,
                   new InotifyReaderNotifyTask(delegate, changed_path));
  }
}

class DirectoryWatcherImpl : public DirectoryWatcher::PlatformDelegate {
 public:
  DirectoryWatcherImpl() : watch_(InotifyReader::kInvalidWatch) {}
  ~DirectoryWatcherImpl();

  virtual bool Watch(const FilePath& path, DirectoryWatcher::Delegate* delegate,
                     bool recursive);

 private:
  
  DirectoryWatcher::Delegate* delegate_;

  
  FilePath path_;

  
  InotifyReader::Watch watch_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryWatcherImpl);
};

DirectoryWatcherImpl::~DirectoryWatcherImpl() {
  if (watch_ != InotifyReader::kInvalidWatch)
    Singleton<InotifyReader>::get()->RemoveWatch(watch_, delegate_);
}

bool DirectoryWatcherImpl::Watch(const FilePath& path,
    DirectoryWatcher::Delegate* delegate, bool recursive) {
  DCHECK(watch_ == InotifyReader::kInvalidWatch);  

  if (recursive) {
    
    
    
    
    
    
    NOTIMPLEMENTED();
    return false;
  }

  delegate_ = delegate;
  path_ = path;
  watch_ = Singleton<InotifyReader>::get()->AddWatch(path, delegate_);

  return watch_ != InotifyReader::kInvalidWatch;
}

}  

DirectoryWatcher::DirectoryWatcher() {
  impl_ = new DirectoryWatcherImpl();
}
