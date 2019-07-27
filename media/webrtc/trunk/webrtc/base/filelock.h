









#ifndef WEBRTC_BASE_FILELOCK_H_
#define WEBRTC_BASE_FILELOCK_H_

#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/scoped_ptr.h"

namespace rtc {

class FileStream;








class FileLock {
 public:
  virtual ~FileLock();

  
  
  static FileLock* TryLock(const std::string& path);
  void Unlock();

 protected:
  FileLock(const std::string& path, FileStream* file);

 private:
  void MaybeUnlock();

  std::string path_;
  scoped_ptr<FileStream> file_;

  DISALLOW_EVIL_CONSTRUCTORS(FileLock);
};

}  

#endif  
