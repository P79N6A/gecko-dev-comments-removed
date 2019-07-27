









#ifndef WEBRTC_BASE_DISKCACHEWIN32_H__
#define WEBRTC_BASE_DISKCACHEWIN32_H__

#include "webrtc/base/diskcache.h"

namespace rtc {

class DiskCacheWin32 : public DiskCache {
 protected:
  virtual bool InitializeEntries();
  virtual bool PurgeFiles();

  virtual bool FileExists(const std::string& filename) const;
  virtual bool DeleteFile(const std::string& filename) const;
};

}

#endif  
