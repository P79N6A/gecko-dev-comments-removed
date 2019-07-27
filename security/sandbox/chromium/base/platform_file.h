



#ifndef BASE_PLATFORM_FILE_H_
#define BASE_PLATFORM_FILE_H_

#include "build/build_config.h"
#if defined(OS_WIN)
#include <windows.h>
#endif

#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/time/time.h"

namespace base {






enum PlatformFileFlags {
  PLATFORM_FILE_OPEN = 1 << 0,             
  PLATFORM_FILE_CREATE = 1 << 1,           
                                           
  PLATFORM_FILE_OPEN_ALWAYS = 1 << 2,      
  PLATFORM_FILE_CREATE_ALWAYS = 1 << 3,    
  PLATFORM_FILE_OPEN_TRUNCATED = 1 << 4,   
                                           
  PLATFORM_FILE_READ = 1 << 5,
  PLATFORM_FILE_WRITE = 1 << 6,
  PLATFORM_FILE_APPEND = 1 << 7,
  PLATFORM_FILE_EXCLUSIVE_READ = 1 << 8,   
                                           
  PLATFORM_FILE_EXCLUSIVE_WRITE = 1 << 9,
  PLATFORM_FILE_ASYNC = 1 << 10,
  PLATFORM_FILE_TEMPORARY = 1 << 11,       
  PLATFORM_FILE_HIDDEN = 1 << 12,          
  PLATFORM_FILE_DELETE_ON_CLOSE = 1 << 13,

  PLATFORM_FILE_WRITE_ATTRIBUTES = 1 << 14,  
  PLATFORM_FILE_ENUMERATE = 1 << 15,         

  PLATFORM_FILE_SHARE_DELETE = 1 << 16,      

  PLATFORM_FILE_TERMINAL_DEVICE = 1 << 17,   
  PLATFORM_FILE_BACKUP_SEMANTICS = 1 << 18,  

  PLATFORM_FILE_EXECUTE = 1 << 19,           
};




enum PlatformFileError {
  PLATFORM_FILE_OK = 0,
  PLATFORM_FILE_ERROR_FAILED = -1,
  PLATFORM_FILE_ERROR_IN_USE = -2,
  PLATFORM_FILE_ERROR_EXISTS = -3,
  PLATFORM_FILE_ERROR_NOT_FOUND = -4,
  PLATFORM_FILE_ERROR_ACCESS_DENIED = -5,
  PLATFORM_FILE_ERROR_TOO_MANY_OPENED = -6,
  PLATFORM_FILE_ERROR_NO_MEMORY = -7,
  PLATFORM_FILE_ERROR_NO_SPACE = -8,
  PLATFORM_FILE_ERROR_NOT_A_DIRECTORY = -9,
  PLATFORM_FILE_ERROR_INVALID_OPERATION = -10,
  PLATFORM_FILE_ERROR_SECURITY = -11,
  PLATFORM_FILE_ERROR_ABORT = -12,
  PLATFORM_FILE_ERROR_NOT_A_FILE = -13,
  PLATFORM_FILE_ERROR_NOT_EMPTY = -14,
  PLATFORM_FILE_ERROR_INVALID_URL = -15,
  PLATFORM_FILE_ERROR_IO = -16,
  
  PLATFORM_FILE_ERROR_MAX = -17
};


enum PlatformFileWhence {
  PLATFORM_FILE_FROM_BEGIN   = 0,
  PLATFORM_FILE_FROM_CURRENT = 1,
  PLATFORM_FILE_FROM_END     = 2
};






struct BASE_EXPORT PlatformFileInfo {
  PlatformFileInfo();
  ~PlatformFileInfo();

  
  int64 size;

  
  bool is_directory;

  
  bool is_symbolic_link;

  
  base::Time last_modified;

  
  base::Time last_accessed;

  
  base::Time creation_time;
};

#if defined(OS_WIN)
typedef HANDLE PlatformFile;
const PlatformFile kInvalidPlatformFileValue = INVALID_HANDLE_VALUE;
PlatformFileError LastErrorToPlatformFileError(DWORD saved_errno);
#elif defined(OS_POSIX)
typedef int PlatformFile;
const PlatformFile kInvalidPlatformFileValue = -1;
PlatformFileError ErrnoToPlatformFileError(int saved_errno);
#endif








BASE_EXPORT PlatformFile CreatePlatformFile(const FilePath& name,
                                            int flags,
                                            bool* created,
                                            PlatformFileError* error);



BASE_EXPORT PlatformFile CreatePlatformFileUnsafe(const FilePath& name,
                                                  int flags,
                                                  bool* created,
                                                  PlatformFileError* error);

BASE_EXPORT FILE* FdopenPlatformFile(PlatformFile file, const char* mode);


BASE_EXPORT bool ClosePlatformFile(PlatformFile file);




BASE_EXPORT int64 SeekPlatformFile(PlatformFile file,
                                   PlatformFileWhence whence,
                                   int64 offset);






BASE_EXPORT int ReadPlatformFile(PlatformFile file, int64 offset,
                                 char* data, int size);


BASE_EXPORT int ReadPlatformFileAtCurrentPos(PlatformFile file,
                                             char* data, int size);




BASE_EXPORT int ReadPlatformFileNoBestEffort(PlatformFile file, int64 offset,
                                             char* data, int size);


BASE_EXPORT int ReadPlatformFileCurPosNoBestEffort(PlatformFile file,
                                                   char* data, int size);







BASE_EXPORT int WritePlatformFile(PlatformFile file, int64 offset,
                                  const char* data, int size);


BASE_EXPORT int WritePlatformFileAtCurrentPos(PlatformFile file,
                                              const char* data, int size);



BASE_EXPORT int WritePlatformFileCurPosNoBestEffort(PlatformFile file,
                                                    const char* data, int size);




BASE_EXPORT bool TruncatePlatformFile(PlatformFile file, int64 length);


BASE_EXPORT bool FlushPlatformFile(PlatformFile file);


BASE_EXPORT bool TouchPlatformFile(PlatformFile file,
                                   const Time& last_access_time,
                                   const Time& last_modified_time);


BASE_EXPORT bool GetPlatformFileInfo(PlatformFile file, PlatformFileInfo* info);





















class BASE_EXPORT PassPlatformFile {
 public:
  explicit PassPlatformFile(PlatformFile* value) : value_(value) {
  }

  
  
  
  PlatformFile ReleaseValue() {
    PlatformFile temp = *value_;
    *value_ = kInvalidPlatformFileValue;
    return temp;
  }

 private:
  PlatformFile* value_;
};

}  

#endif  
