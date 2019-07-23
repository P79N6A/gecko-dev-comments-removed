



#ifndef BASE_SHARED_MEMORY_H_
#define BASE_SHARED_MEMORY_H_

#include "build/build_config.h"

#if defined(OS_POSIX)
#include <sys/types.h>
#include <semaphore.h>
#include "base/file_descriptor_posix.h"
#endif
#include <string>

#include "base/basictypes.h"
#include "base/process.h"

namespace base {



#if defined(OS_WIN)
typedef HANDLE SharedMemoryHandle;
typedef HANDLE SharedMemoryLock;
#elif defined(OS_POSIX)


typedef FileDescriptor SharedMemoryHandle;
typedef ino_t SharedMemoryId;



#endif



class SharedMemory {
 public:
  
  SharedMemory();

  
  
  SharedMemory(SharedMemoryHandle handle, bool read_only);

  
  
  
  SharedMemory(SharedMemoryHandle handle, bool read_only,
      base::ProcessHandle process);

  
  ~SharedMemory();

  
  
  static bool IsHandleValid(const SharedMemoryHandle& handle);

  
  static SharedMemoryHandle NULLHandle();

  
  
  
  
  
  
#ifdef CHROMIUM_MOZILLA_BUILD
  bool Create(const std::string& name, bool read_only, bool open_existing,
#else
  bool Create(const std::wstring& name, bool read_only, bool open_existing,
#endif
              size_t size);

  
  
  bool Delete(const std::wstring& name);

  
  
  
  
  bool Open(const std::wstring& name, bool read_only);

  
  
  
  bool Map(size_t bytes);

  
  
  
  bool Unmap();

  
  
  
  
  
  size_t max_size() const { return max_size_; }

  
  
  void *memory() const { return memory_; }

  
  
  
  SharedMemoryHandle handle() const;

#if defined(OS_POSIX)
  
  
  
  
  SharedMemoryId id() const { return inode_; }
#endif

  
  
  void Close();

  
  
  
  
  
  
  bool ShareToProcess(base::ProcessHandle process,
                      SharedMemoryHandle* new_handle) {
    return ShareToProcessCommon(process, new_handle, false);
  }

  
  
  
  
  
  
  bool GiveToProcess(ProcessHandle process,
                     SharedMemoryHandle* new_handle) {
    return ShareToProcessCommon(process, new_handle, true);
  }

  
  
  
  
  
  
  
  
  
  void Lock();

  
  void Unlock();

 private:
#if defined(OS_POSIX)
  bool CreateOrOpen(const std::wstring &name, int posix_flags, size_t size);
  bool FilenameForMemoryName(const std::wstring &memname,
                             std::wstring *filename);
  void LockOrUnlockCommon(int function);

#endif
  bool ShareToProcessCommon(ProcessHandle process,
                            SharedMemoryHandle* new_handle,
                            bool close_self);

#if defined(OS_WIN)
  std::wstring       name_;
  HANDLE             mapped_file_;
#elif defined(OS_POSIX)
  int                mapped_file_;
  ino_t              inode_;
#endif
  void*              memory_;
  bool               read_only_;
  size_t             max_size_;
#if !defined(OS_POSIX)
  SharedMemoryLock   lock_;
#endif

  DISALLOW_EVIL_CONSTRUCTORS(SharedMemory);
};



class SharedMemoryAutoLock {
 public:
  explicit SharedMemoryAutoLock(SharedMemory* shared_memory)
      : shared_memory_(shared_memory) {
    shared_memory_->Lock();
  }

  ~SharedMemoryAutoLock() {
    shared_memory_->Unlock();
  }

 private:
  SharedMemory* shared_memory_;
  DISALLOW_EVIL_CONSTRUCTORS(SharedMemoryAutoLock);
};

}  

#endif
