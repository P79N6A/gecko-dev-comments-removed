









#include "webrtc/base/filelock.h"

#include "webrtc/base/fileutils.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/pathutils.h"
#include "webrtc/base/stream.h"

namespace rtc {

FileLock::FileLock(const std::string& path, FileStream* file)
    : path_(path), file_(file) {
}

FileLock::~FileLock() {
  MaybeUnlock();
}

void FileLock::Unlock() {
  LOG_F(LS_INFO);
  MaybeUnlock();
}

void FileLock::MaybeUnlock() {
  if (file_) {
    LOG(LS_INFO) << "Unlocking:" << path_;
    file_->Close();
    Filesystem::DeleteFile(path_);
    file_.reset();
  }
}

FileLock* FileLock::TryLock(const std::string& path) {
  FileStream* stream = new FileStream();
  bool ok = false;
#if defined(WEBRTC_WIN)
  
  ok = stream->OpenShare(path, "a", _SH_DENYRW, NULL);
#else 
  ok = stream->Open(path, "a", NULL) && stream->TryLock();
#endif
  if (ok) {
    return new FileLock(path, stream);
  } else {
    
    
    
    delete stream;
    return NULL;
  }
}

}  
