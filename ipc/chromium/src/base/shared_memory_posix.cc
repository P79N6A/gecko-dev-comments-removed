



#include "base/shared_memory.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base/file_util.h"
#include "base/logging.h"
#include "base/platform_thread.h"
#include "base/string_util.h"
#include "mozilla/UniquePtr.h"

namespace base {

SharedMemory::SharedMemory()
    : mapped_file_(-1),
      inode_(0),
      memory_(NULL),
      read_only_(false),
      max_size_(0) {
}

SharedMemory::SharedMemory(SharedMemoryHandle handle, bool read_only)
    : mapped_file_(handle.fd),
      inode_(0),
      memory_(NULL),
      read_only_(read_only),
      max_size_(0) {
  struct stat st;
  if (fstat(handle.fd, &st) == 0) {
    
    
    inode_ = st.st_ino;
  }
}

SharedMemory::SharedMemory(SharedMemoryHandle handle, bool read_only,
                           ProcessHandle process)
    : mapped_file_(handle.fd),
      memory_(NULL),
      read_only_(read_only),
      max_size_(0) {
  
  
  NOTREACHED();
}

SharedMemory::~SharedMemory() {
  Close();
}


bool SharedMemory::IsHandleValid(const SharedMemoryHandle& handle) {
  return handle.fd >= 0;
}


SharedMemoryHandle SharedMemory::NULLHandle() {
  return SharedMemoryHandle();
}

bool SharedMemory::Create(const std::string &cname, bool read_only,
                          bool open_existing, size_t size) {
  read_only_ = read_only;

  std::wstring name = UTF8ToWide(cname);

  int posix_flags = 0;
  posix_flags |= read_only ? O_RDONLY : O_RDWR;
  if (!open_existing || mapped_file_ <= 0)
    posix_flags |= O_CREAT;

  if (!CreateOrOpen(name, posix_flags, size))
    return false;

  max_size_ = size;
  return true;
}




bool SharedMemory::Delete(const std::wstring& name) {
  std::wstring mem_filename;
  if (FilenameForMemoryName(name, &mem_filename) == false)
    return false;

  FilePath path(WideToUTF8(mem_filename));
  if (file_util::PathExists(path)) {
    return file_util::Delete(path);
  }

  
  return true;
}

bool SharedMemory::Open(const std::wstring &name, bool read_only) {
  read_only_ = read_only;

  int posix_flags = 0;
  posix_flags |= read_only ? O_RDONLY : O_RDWR;

  return CreateOrOpen(name, posix_flags, 0);
}




bool SharedMemory::FilenameForMemoryName(const std::wstring &memname,
                                         std::wstring *filename) {
  std::wstring mem_filename;

  
  
  DCHECK(memname.find_first_of(L"/") == std::string::npos);
  DCHECK(memname.find_first_of(L"\0") == std::string::npos);

  FilePath temp_dir;
  if (file_util::GetShmemTempDir(&temp_dir) == false)
    return false;

  mem_filename = UTF8ToWide(temp_dir.value());
  file_util::AppendToPath(&mem_filename, L"com.google.chrome.shmem." + memname);
  *filename = mem_filename;
  return true;
}

namespace {


class ScopedFILEClose {
 public:
  inline void operator()(FILE* x) const {
    if (x) {
      fclose(x);
    }
  }
};

typedef mozilla::UniquePtr<FILE, ScopedFILEClose> ScopedFILE;

}







bool SharedMemory::CreateOrOpen(const std::wstring &name,
                                int posix_flags, size_t size) {
  DCHECK(mapped_file_ == -1);

  ScopedFILE file_closer;
  FILE *fp;

  if (name == L"") {
    
    DCHECK(posix_flags & (O_RDWR | O_WRONLY));

    FilePath path;
    fp = file_util::CreateAndOpenTemporaryShmemFile(&path);

    
    
    
    file_util::Delete(path);
  } else {
    std::wstring mem_filename;
    if (FilenameForMemoryName(name, &mem_filename) == false)
      return false;

    std::string mode;
    switch (posix_flags) {
      case (O_RDWR | O_CREAT):
        
        mode = "a+";
        break;
      case O_RDWR:
        mode = "r+";
        break;
      case O_RDONLY:
        mode = "r";
        break;
      default:
        NOTIMPLEMENTED();
        break;
    }

    fp = file_util::OpenFile(mem_filename, mode.c_str());
  }

  if (fp == NULL)
    return false;
  file_closer.reset(fp);  

  
  
  
  if (size && (posix_flags & (O_RDWR | O_CREAT))) {
    
    struct stat stat;
    if (fstat(fileno(fp), &stat) != 0)
      return false;
    size_t current_size = stat.st_size;
    if (current_size != size) {
      if (ftruncate(fileno(fp), size) != 0)
        return false;
      if (fseeko(fp, size, SEEK_SET) != 0)
        return false;
    }
  }

  mapped_file_ = dup(fileno(fp));
  DCHECK(mapped_file_ >= 0);

  struct stat st;
  if (fstat(mapped_file_, &st))
    NOTREACHED();
  inode_ = st.st_ino;

  return true;
}

bool SharedMemory::Map(size_t bytes) {
  if (mapped_file_ == -1)
    return false;

  memory_ = mmap(NULL, bytes, PROT_READ | (read_only_ ? 0 : PROT_WRITE),
                 MAP_SHARED, mapped_file_, 0);

  if (memory_)
    max_size_ = bytes;

  bool mmap_succeeded = (memory_ != (void*)-1);
  DCHECK(mmap_succeeded) << "Call to mmap failed, errno=" << errno;
  return mmap_succeeded;
}

bool SharedMemory::Unmap() {
  if (memory_ == NULL)
    return false;

  munmap(memory_, max_size_);
  memory_ = NULL;
  max_size_ = 0;
  return true;
}

bool SharedMemory::ShareToProcessCommon(ProcessId processId,
                                        SharedMemoryHandle *new_handle,
                                        bool close_self) {
  const int new_fd = dup(mapped_file_);
  DCHECK(new_fd >= -1);
  new_handle->fd = new_fd;
  new_handle->auto_close = true;

  if (close_self)
    Close();

  return true;
}


void SharedMemory::Close() {
  Unmap();

  if (mapped_file_ > 0) {
    close(mapped_file_);
    mapped_file_ = -1;
  }
}

#ifdef ANDROID
void SharedMemory::LockOrUnlockCommon(int function) {
  DCHECK(mapped_file_ >= 0);
  struct flock lockreq;
  lockreq.l_type = function;
  lockreq.l_whence = SEEK_SET;
  lockreq.l_start = 0;
  lockreq.l_len = 0;
  while (fcntl(mapped_file_, F_SETLKW, &lockreq) < 0) {
    if (errno == EINTR) {
      continue;
    } else if (errno == ENOLCK) {
      
      PlatformThread::Sleep(500);
      continue;
    } else {
      NOTREACHED() << "lockf() failed."
                   << " function:" << function
                   << " fd:" << mapped_file_
                   << " errno:" << errno
                   << " msg:" << strerror(errno);
    }
  }
}

void SharedMemory::Lock() {
  LockOrUnlockCommon(F_WRLCK);
}

void SharedMemory::Unlock() {
  LockOrUnlockCommon(F_UNLCK);
}
#else
void SharedMemory::LockOrUnlockCommon(int function) {
  DCHECK(mapped_file_ >= 0);
  while (lockf(mapped_file_, function, 0) < 0) {
    if (errno == EINTR) {
      continue;
    } else if (errno == ENOLCK) {
      
      PlatformThread::Sleep(500);
      continue;
    } else {
      NOTREACHED() << "lockf() failed."
                   << " function:" << function
                   << " fd:" << mapped_file_
                   << " errno:" << errno
                   << " msg:" << strerror(errno);
    }
  }
}

void SharedMemory::Lock() {
  LockOrUnlockCommon(F_LOCK);
}

void SharedMemory::Unlock() {
  LockOrUnlockCommon(F_ULOCK);
}
#endif

SharedMemoryHandle SharedMemory::handle() const {
  return FileDescriptor(mapped_file_, false);
}

}  
