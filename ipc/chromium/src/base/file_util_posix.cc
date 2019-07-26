



#include "base/file_util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#ifndef ANDROID
#include <fts.h>
#endif
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#define _DARWIN_USE_64_BIT_INODE
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/eintr_wrapper.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/time.h"

namespace file_util {

#if defined(GOOGLE_CHROME_BUILD)
static const char* kTempFileName = "com.google.chrome.XXXXXX";
#else
static const char* kTempFileName = "org.chromium.XXXXXX";
#endif

std::wstring GetDirectoryFromPath(const std::wstring& path) {
  if (EndsWithSeparator(path)) {
    std::wstring dir = path;
    TrimTrailingSeparator(&dir);
    return dir;
  } else {
    char full_path[PATH_MAX];
    base::strlcpy(full_path, WideToUTF8(path).c_str(), arraysize(full_path));
    return UTF8ToWide(dirname(full_path));
  }
}

bool AbsolutePath(FilePath* path) {
  char full_path[PATH_MAX];
  if (realpath(path->value().c_str(), full_path) == NULL)
    return false;
  *path = FilePath(full_path);
  return true;
}

int CountFilesCreatedAfter(const FilePath& path,
                           const base::Time& comparison_time) {
  int file_count = 0;

  DIR* dir = opendir(path.value().c_str());
  if (dir) {
    struct dirent ent_buf;
    struct dirent* ent;
    while (readdir_r(dir, &ent_buf, &ent) == 0 && ent) {
      if ((strcmp(ent->d_name, ".") == 0) ||
          (strcmp(ent->d_name, "..") == 0))
        continue;

      struct stat st;
      int test = stat(path.Append(ent->d_name).value().c_str(), &st);
      if (test != 0) {
        LOG(ERROR) << "stat failed: " << strerror(errno);
        continue;
      }
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (st.st_ctime >= comparison_time.ToTimeT())
        ++file_count;
    }
    closedir(dir);
  }
  return file_count;
}





bool Delete(const FilePath& path, bool recursive) {
  const char* path_str = path.value().c_str();
  struct stat file_info;
  int test = stat(path_str, &file_info);
  if (test != 0) {
    
    bool ret = (errno == ENOENT || errno == ENOTDIR);
    return ret;
  }
  if (!S_ISDIR(file_info.st_mode))
    return (unlink(path_str) == 0);
  if (!recursive)
    return (rmdir(path_str) == 0);

#ifdef ANDROID
  
  return false;
#else
  bool success = true;
  int ftsflags = FTS_PHYSICAL | FTS_NOSTAT;
  char top_dir[PATH_MAX];
  if (base::strlcpy(top_dir, path_str,
                    arraysize(top_dir)) >= arraysize(top_dir)) {
    return false;
  }
  char* dir_list[2] = { top_dir, NULL };
  FTS* fts = fts_open(dir_list, ftsflags, NULL);
  if (fts) {
    FTSENT* fts_ent = fts_read(fts);
    while (success && fts_ent != NULL) {
      switch (fts_ent->fts_info) {
        case FTS_DNR:
        case FTS_ERR:
          
          success = false;
          continue;
          break;
        case FTS_DP:
          success = (rmdir(fts_ent->fts_accpath) == 0);
          break;
        case FTS_D:
          break;
        case FTS_NSOK:
        case FTS_F:
        case FTS_SL:
        case FTS_SLNONE:
          success = (unlink(fts_ent->fts_accpath) == 0);
          break;
        default:
          DCHECK(false);
          break;
      }
      fts_ent = fts_read(fts);
    }
    fts_close(fts);
  }
  return success;
#endif
}

bool Move(const FilePath& from_path, const FilePath& to_path) {
  if (rename(from_path.value().c_str(), to_path.value().c_str()) == 0)
    return true;

  if (!CopyDirectory(from_path, to_path, true))
    return false;

  Delete(from_path, true);
  return true;
}

bool CopyDirectory(const FilePath& from_path,
                   const FilePath& to_path,
                   bool recursive) {
  
  
  
  
  DCHECK(to_path.value().find('*') == std::string::npos);
  DCHECK(from_path.value().find('*') == std::string::npos);

  char top_dir[PATH_MAX];
  if (base::strlcpy(top_dir, from_path.value().c_str(),
                    arraysize(top_dir)) >= arraysize(top_dir)) {
    return false;
  }

#ifdef ANDROID
  
  return false;
#else
  char* dir_list[] = { top_dir, NULL };
  FTS* fts = fts_open(dir_list, FTS_PHYSICAL | FTS_NOSTAT, NULL);
  if (!fts) {
    LOG(ERROR) << "fts_open failed: " << strerror(errno);
    return false;
  }

  int error = 0;
  FTSENT* ent;
  while (!error && (ent = fts_read(fts)) != NULL) {
    
    
    std::string suffix(&ent->fts_path[from_path.value().size()]);
    
    if (!suffix.empty()) {
      DCHECK_EQ('/', suffix[0]);
      suffix.erase(0, 1);
    }
    const FilePath target_path = to_path.Append(suffix);
    switch (ent->fts_info) {
      case FTS_D:  
        
        
        if (!recursive && ent->fts_level > 0) {
          if (fts_set(fts, ent, FTS_SKIP) != 0)
            error = errno;
          continue;
        }

        
        
        if (mkdir(target_path.value().c_str(), 0777) != 0) {
          if (errno != EEXIST)
            error = errno;
        }
        break;
      case FTS_F:     
      case FTS_NSOK:  
        errno = 0;
        if (!CopyFile(FilePath(ent->fts_path), target_path))
          error = errno ? errno : EINVAL;
        break;
      case FTS_DP:   
      case FTS_DOT:  
        
        continue;
      case FTS_DC:   
        
        if (fts_set(fts, ent, FTS_SKIP) != 0)
          error = errno;
        break;
      case FTS_DNR:  
      case FTS_ERR:  
      case FTS_NS:   
        
        error = ent->fts_errno;
        break;
      case FTS_SL:      
      case FTS_SLNONE:  
        LOG(WARNING) << "CopyDirectory() skipping symbolic link: " <<
            ent->fts_path;
        continue;
      case FTS_DEFAULT:  
        LOG(WARNING) << "CopyDirectory() skipping file of unknown type: " <<
            ent->fts_path;
        continue;
      default:
        NOTREACHED();
        continue;  
    }
  }
  
  if (!error && errno != 0)
    error = errno;

  if (!fts_close(fts)) {
    
    
    if (!error)
      error = errno;
  }

  if (error) {
    LOG(ERROR) << "CopyDirectory(): " << strerror(error);
    return false;
  }
  return true;
#endif
}

bool PathExists(const FilePath& path) {
  struct stat file_info;
  return (stat(path.value().c_str(), &file_info) == 0);
}

bool PathIsWritable(const FilePath& path) {
  FilePath test_path(path);
  struct stat file_info;
  if (stat(test_path.value().c_str(), &file_info) != 0) {
    
    test_path = test_path.DirName();
    
    
    if (stat(test_path.value().c_str(), &file_info) != 0)
      return false;
  }
  if (S_IWOTH & file_info.st_mode)
    return true;
  if (getegid() == file_info.st_gid && (S_IWGRP & file_info.st_mode))
    return true;
  if (geteuid() == file_info.st_uid && (S_IWUSR & file_info.st_mode))
    return true;
  return false;
}

bool DirectoryExists(const FilePath& path) {
  struct stat file_info;
  if (stat(path.value().c_str(), &file_info) == 0)
    return S_ISDIR(file_info.st_mode);
  return false;
}


#if 0
bool GetFileCreationLocalTimeFromHandle(int fd,
                                        LPSYSTEMTIME creation_time) {
  if (!file_handle)
    return false;

  FILETIME utc_filetime;
  if (!GetFileTime(file_handle, &utc_filetime, NULL, NULL))
    return false;

  FILETIME local_filetime;
  if (!FileTimeToLocalFileTime(&utc_filetime, &local_filetime))
    return false;

  return !!FileTimeToSystemTime(&local_filetime, creation_time);
}

bool GetFileCreationLocalTime(const std::string& filename,
                              LPSYSTEMTIME creation_time) {
  ScopedHandle file_handle(
      CreateFile(filename.c_str(), GENERIC_READ,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
  return GetFileCreationLocalTimeFromHandle(file_handle.Get(), creation_time);
}
#endif

bool ReadFromFD(int fd, char* buffer, size_t bytes) {
  size_t total_read = 0;
  while (total_read < bytes) {
    ssize_t bytes_read =
        HANDLE_EINTR(read(fd, buffer + total_read, bytes - total_read));
    if (bytes_read <= 0)
      break;
    total_read += bytes_read;
  }
  return total_read == bytes;
}






int CreateAndOpenFdForTemporaryFile(FilePath directory, FilePath* path) {
  *path = directory.Append(kTempFileName);
  const std::string& tmpdir_string = path->value();
  
  char* buffer = const_cast<char*>(tmpdir_string.c_str());

  return mkstemp(buffer);
}

bool CreateTemporaryFileName(FilePath* path) {
  FilePath directory;
  if (!GetTempDir(&directory))
    return false;
  int fd = CreateAndOpenFdForTemporaryFile(directory, path);
  if (fd < 0)
    return false;
  close(fd);
  return true;
}

FILE* CreateAndOpenTemporaryShmemFile(FilePath* path) {
  FilePath directory;
  if (!GetShmemTempDir(&directory))
    return NULL;

  return CreateAndOpenTemporaryFileInDir(directory, path);
}

FILE* CreateAndOpenTemporaryFileInDir(const FilePath& dir, FilePath* path) {
  int fd = CreateAndOpenFdForTemporaryFile(dir, path);
  if (fd < 0)
    return NULL;

  return fdopen(fd, "a+");
}

bool CreateTemporaryFileNameInDir(const std::wstring& dir,
                                  std::wstring* temp_file) {
  
  NOTREACHED();
  return false;
}

bool CreateNewTempDirectory(const FilePath::StringType& prefix,
                            FilePath* new_temp_path) {
  FilePath tmpdir;
  if (!GetTempDir(&tmpdir))
    return false;
  tmpdir = tmpdir.Append(kTempFileName);
  std::string tmpdir_string = tmpdir.value();
  
  char* buffer = const_cast<char*>(tmpdir_string.c_str());
#ifdef ANDROID
  char* dtemp = NULL;
#else
  char* dtemp = mkdtemp(buffer);
#endif
  if (!dtemp)
    return false;
  *new_temp_path = FilePath(dtemp);
  return true;
}

bool CreateDirectory(const FilePath& full_path) {
  std::vector<FilePath> subpaths;

  
  FilePath last_path = full_path;
  subpaths.push_back(full_path);
  for (FilePath path = full_path.DirName();
       path.value() != last_path.value(); path = path.DirName()) {
    subpaths.push_back(path);
    last_path = path;
  }

  
  for (std::vector<FilePath>::reverse_iterator i = subpaths.rbegin();
       i != subpaths.rend(); ++i) {
    if (!DirectoryExists(*i)) {
      if (mkdir(i->value().c_str(), 0777) != 0)
        return false;
    }
  }
  return true;
}

bool GetFileInfo(const FilePath& file_path, FileInfo* results) {
  struct stat file_info;
  if (stat(file_path.value().c_str(), &file_info) != 0)
    return false;
  results->is_directory = S_ISDIR(file_info.st_mode);
  results->size = file_info.st_size;
  return true;
}

FILE* OpenFile(const std::string& filename, const char* mode) {
  return OpenFile(FilePath(filename), mode);
}

FILE* OpenFile(const FilePath& filename, const char* mode) {
  return fopen(filename.value().c_str(), mode);
}

int ReadFile(const FilePath& filename, char* data, int size) {
  int fd = open(filename.value().c_str(), O_RDONLY);
  if (fd < 0)
    return -1;

  int ret_value = HANDLE_EINTR(read(fd, data, size));
  HANDLE_EINTR(close(fd));
  return ret_value;
}

int WriteFile(const FilePath& filename, const char* data, int size) {
  int fd = creat(filename.value().c_str(), 0666);
  if (fd < 0)
    return -1;

  
  ssize_t bytes_written_total = 0;
  do {
    ssize_t bytes_written_partial =
      HANDLE_EINTR(write(fd, data + bytes_written_total,
                         size - bytes_written_total));
    if (bytes_written_partial < 0) {
      HANDLE_EINTR(close(fd));
      return -1;
    }
    bytes_written_total += bytes_written_partial;
  } while (bytes_written_total < size);

  HANDLE_EINTR(close(fd));
  return bytes_written_total;
}


bool GetCurrentDirectory(FilePath* dir) {
  char system_buffer[PATH_MAX] = "";
  if (!getcwd(system_buffer, sizeof(system_buffer))) {
    NOTREACHED();
    return false;
  }
  *dir = FilePath(system_buffer);
  return true;
}


bool SetCurrentDirectory(const FilePath& path) {
  int ret = chdir(path.value().c_str());
  return !ret;
}

#if !defined(OS_MACOSX)
bool GetTempDir(FilePath* path) {
  const char* tmp = getenv("TMPDIR");
  if (tmp)
    *path = FilePath(tmp);
  else
    *path = FilePath("/tmp");
  return true;
}

bool GetShmemTempDir(FilePath* path) {
#if defined(OS_LINUX) && !defined(ANDROID)
  *path = FilePath("/dev/shm");
  return true;
#else
  return GetTempDir(path);
#endif
}

bool CopyFile(const FilePath& from_path, const FilePath& to_path) {
  int infile = open(from_path.value().c_str(), O_RDONLY);
  if (infile < 0)
    return false;

  int outfile = creat(to_path.value().c_str(), 0666);
  if (outfile < 0) {
    close(infile);
    return false;
  }

  const size_t kBufferSize = 32768;
  std::vector<char> buffer(kBufferSize);
  bool result = true;

  while (result) {
    ssize_t bytes_read = HANDLE_EINTR(read(infile, &buffer[0], buffer.size()));
    if (bytes_read < 0) {
      result = false;
      break;
    }
    if (bytes_read == 0)
      break;
    
    ssize_t bytes_written_per_read = 0;
    do {
      ssize_t bytes_written_partial = HANDLE_EINTR(write(
          outfile,
          &buffer[bytes_written_per_read],
          bytes_read - bytes_written_per_read));
      if (bytes_written_partial < 0) {
        result = false;
        break;
      }
      bytes_written_per_read += bytes_written_partial;
    } while (bytes_written_per_read < bytes_read);
  }

  if (HANDLE_EINTR(close(infile)) < 0)
    result = false;
  if (HANDLE_EINTR(close(outfile)) < 0)
    result = false;

  return result;
}
#endif 




FileEnumerator::FileEnumerator(const FilePath& root_path,
                               bool recursive,
                               FileEnumerator::FILE_TYPE file_type)
    : recursive_(recursive),
      file_type_(file_type),
      is_in_find_op_(false),
      fts_(NULL) {
  pending_paths_.push(root_path);
}

FileEnumerator::FileEnumerator(const FilePath& root_path,
                               bool recursive,
                               FileEnumerator::FILE_TYPE file_type,
                               const FilePath::StringType& pattern)
    : recursive_(recursive),
      file_type_(file_type),
      pattern_(root_path.value()),
      is_in_find_op_(false),
      fts_(NULL) {
  
  
  
  pattern_ = pattern_.Append(pattern);
  pending_paths_.push(root_path);
}

FileEnumerator::~FileEnumerator() {
#ifndef ANDROID
  if (fts_)
    fts_close(fts_);
#endif
}

void FileEnumerator::GetFindInfo(FindInfo* info) {
  DCHECK(info);

  if (!is_in_find_op_)
    return;

#ifndef ANDROID
  memcpy(&(info->stat), fts_ent_->fts_statp, sizeof(info->stat));
  info->filename.assign(fts_ent_->fts_name);
#endif
}





FilePath FileEnumerator::Next() {
#ifdef ANDROID
  return FilePath();
#else
  if (!is_in_find_op_) {
    if (pending_paths_.empty())
      return FilePath();

    
    root_path_ = pending_paths_.top();
    root_path_ = root_path_.StripTrailingSeparators();
    pending_paths_.pop();

    
    int ftsflags = FTS_LOGICAL;
    char top_dir[PATH_MAX];
    base::strlcpy(top_dir, root_path_.value().c_str(), arraysize(top_dir));
    char* dir_list[2] = { top_dir, NULL };
    fts_ = fts_open(dir_list, ftsflags, NULL);
    if (!fts_)
      return Next();
    is_in_find_op_ = true;
  }

  fts_ent_ = fts_read(fts_);
  if (fts_ent_ == NULL) {
    fts_close(fts_);
    fts_ = NULL;
    is_in_find_op_ = false;
    return Next();
  }

  
  if (fts_ent_->fts_level == 0)
    return Next();

  
  
  if (fts_ent_->fts_level == 1 && pattern_.value().length() > 0) {
    if (fnmatch(pattern_.value().c_str(), fts_ent_->fts_path, 0) != 0) {
      if (fts_ent_->fts_info == FTS_D)
        fts_set(fts_, fts_ent_, FTS_SKIP);
      return Next();
    }
  }

  FilePath cur_file(fts_ent_->fts_path);
  if (fts_ent_->fts_info == FTS_D) {
    
    if (!recursive_)
      fts_set(fts_, fts_ent_, FTS_SKIP);
    return (file_type_ & FileEnumerator::DIRECTORIES) ? cur_file : Next();
  } else if (fts_ent_->fts_info == FTS_F) {
    return (file_type_ & FileEnumerator::FILES) ? cur_file : Next();
  }
  
  return Next();
#endif
}




MemoryMappedFile::MemoryMappedFile()
    : file_(-1),
      data_(NULL),
      length_(0) {
}

bool MemoryMappedFile::MapFileToMemory(const FilePath& file_name) {
  file_ = open(file_name.value().c_str(), O_RDONLY);
  if (file_ == -1)
    return false;

  struct stat file_stat;
  if (fstat(file_, &file_stat) == -1)
    return false;
  length_ = file_stat.st_size;

  data_ = static_cast<uint8_t*>(
      mmap(NULL, length_, PROT_READ, MAP_SHARED, file_, 0));
  if (data_ == MAP_FAILED)
    data_ = NULL;
  return data_ != NULL;
}

void MemoryMappedFile::CloseHandles() {
  if (data_ != NULL)
    munmap(data_, length_);
  if (file_ != -1)
    close(file_);

  data_ = NULL;
  length_ = 0;
  file_ = -1;
}

} 
