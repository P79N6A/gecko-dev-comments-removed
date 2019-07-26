





#include "mozilla/Mutex.h"
#include "mozilla/Scoped.h"

#include <algorithm>
#include <vector>

#include "PoisonIOInterposer.h"


#if defined(XP_WIN32)
#include <io.h>
inline intptr_t FileDescriptorToID(int aFd) {
  return _get_osfhandle(aFd);
}
#else
inline intptr_t FileDescriptorToID(int aFd) {
  return aFd;
}
#endif 

using namespace mozilla;

namespace {
struct DebugFilesAutoLockTraits {
  typedef PRLock *type;
  const static type empty() {
    return nullptr;
  }
  const static void release(type aL) {
    PR_Unlock(aL);
  }
};

class DebugFilesAutoLock : public Scoped<DebugFilesAutoLockTraits> {
  static PRLock *Lock;
public:
  static void Clear();
  static PRLock *getDebugFileIDsLock() {
    
    
    
    
    
    
    if (!Lock) {
      Lock = PR_NewLock();
    }

    
    
    return Lock;
  }

  DebugFilesAutoLock() :
    Scoped<DebugFilesAutoLockTraits>(getDebugFileIDsLock()) {
    PR_Lock(get());
  }
};

PRLock *DebugFilesAutoLock::Lock;
void DebugFilesAutoLock::Clear() {
  MOZ_ASSERT(Lock != nullptr);
  Lock = nullptr;
}



std::vector<intptr_t>* getDebugFileIDs() {
  PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(DebugFilesAutoLock::getDebugFileIDsLock());
  
  
  static std::vector<intptr_t> *DebugFileIDs = new std::vector<intptr_t>();
  return DebugFileIDs;
}

} 

namespace mozilla{



bool IsDebugFile(intptr_t aFileID) {
  DebugFilesAutoLock lockedScope;

  std::vector<intptr_t> &Vec = *getDebugFileIDs();
  return std::find(Vec.begin(), Vec.end(), aFileID) != Vec.end();
}

















} 

extern "C" {

  void MozillaRegisterDebugFD(int fd) {
    intptr_t fileId = FileDescriptorToID(fd);
    DebugFilesAutoLock lockedScope;
    std::vector<intptr_t> &Vec = *getDebugFileIDs();
    MOZ_ASSERT(std::find(Vec.begin(), Vec.end(), fileId) == Vec.end());
    Vec.push_back(fileId);
  }

  void MozillaRegisterDebugFILE(FILE *f) {
    int fd = fileno(f);
    if (fd == 1 || fd == 2) {
      return;
    }
    MozillaRegisterDebugFD(fd);
  }

  void MozillaUnRegisterDebugFD(int fd) {
    DebugFilesAutoLock lockedScope;
    intptr_t fileId = FileDescriptorToID(fd);
    std::vector<intptr_t> &Vec = *getDebugFileIDs();
    std::vector<intptr_t>::iterator i =
      std::find(Vec.begin(), Vec.end(), fileId);
    MOZ_ASSERT(i != Vec.end());
    Vec.erase(i);
  }

  void MozillaUnRegisterDebugFILE(FILE *f) {
    int fd = fileno(f);
    if (fd == 1 || fd == 2) {
      return;
    }
    fflush(f);
    MozillaUnRegisterDebugFD(fd);
  }

}