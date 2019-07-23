






#ifndef BASE_FILE_UTIL_H_
#define BASE_FILE_UTIL_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <fts.h>
#include <sys/stat.h>
#endif

#include <stdio.h>

#include <stack>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/file_path.h"

namespace base {
class Time;
}

namespace file_util {





void PathComponents(const FilePath& path,
                    std::vector<FilePath::StringType>* components);
#if defined(OS_WIN)

void PathComponents(const std::wstring& path,
                    std::vector<std::wstring>* components);
#endif


bool EndsWithSeparator(const FilePath& path);

bool EndsWithSeparator(std::wstring* path);
bool EndsWithSeparator(const std::wstring& path);



bool EnsureEndsWithSeparator(FilePath* path);




void TrimTrailingSeparator(std::wstring* dir);





void UpOneDirectory(std::wstring* dir);




void UpOneDirectoryOrEmpty(std::wstring* dir);



std::wstring GetFilenameFromPath(const std::wstring& path);


FilePath::StringType GetFileExtensionFromPath(const FilePath& path);

std::wstring GetFileExtensionFromPath(const std::wstring& path);










std::wstring GetDirectoryFromPath(const std::wstring& path);


void AppendToPath(std::wstring* path, const std::wstring& new_ending);



bool AbsolutePath(FilePath* path);

bool AbsolutePath(std::wstring* path);



bool ContainsPath(const FilePath& parent, const FilePath& child);


void InsertBeforeExtension(FilePath* path, const FilePath::StringType& suffix);


void ReplaceExtension(FilePath* file_name,
                      const FilePath::StringType& extension);

#if defined(OS_WIN)

void InsertBeforeExtension(std::wstring* path, const std::wstring& suffix);
void ReplaceExtension(std::wstring* file_name, const std::wstring& extension);
#endif








void ReplaceIllegalCharacters(std::wstring* file_name, int replace_char);











int CountFilesCreatedAfter(const FilePath& path,
                           const base::Time& file_time);









bool Delete(const FilePath& path, bool recursive);

bool Delete(const std::wstring& path, bool recursive);





bool Move(const FilePath& from_path, const FilePath& to_path);

bool Move(const std::wstring& from_path, const std::wstring& to_path);


bool CopyFile(const FilePath& from_path, const FilePath& to_path);

bool CopyFile(const std::wstring& from_path, const std::wstring& to_path);








bool CopyDirectory(const FilePath& from_path, const FilePath& to_path,
                   bool recursive);

bool CopyDirectory(const std::wstring& from_path, const std::wstring& to_path,
                   bool recursive);



bool PathExists(const FilePath& path);

bool PathExists(const std::wstring& path);


bool PathIsWritable(const FilePath& path);

bool PathIsWritable(const std::wstring& path);


bool DirectoryExists(const FilePath& path);

bool DirectoryExists(const std::wstring& path);

#if defined(OS_WIN)



bool GetFileCreationLocalTime(const std::wstring& filename,
                              LPSYSTEMTIME creation_time);


bool GetFileCreationLocalTimeFromHandle(HANDLE file_handle,
                                        LPSYSTEMTIME creation_time);
#endif  



bool ContentsEqual(const FilePath& filename1,
                   const FilePath& filename2);

bool ContentsEqual(const std::wstring& filename1,
                   const std::wstring& filename2);



bool ReadFileToString(const FilePath& path, std::string* contents);

bool ReadFileToString(const std::wstring& path, std::string* contents);

#if defined(OS_POSIX)



bool ReadFromFD(int fd, char* buffer, size_t bytes);
#endif  

#if defined(OS_WIN)



bool ResolveShortcut(FilePath* path);

bool ResolveShortcut(std::wstring* path);










bool CreateShortcutLink(const wchar_t *source, const wchar_t *destination,
                        const wchar_t *working_dir, const wchar_t *arguments,
                        const wchar_t *description, const wchar_t *icon,
                        int icon_index);








bool UpdateShortcutLink(const wchar_t *source, const wchar_t *destination,
                        const wchar_t *working_dir, const wchar_t *arguments,
                        const wchar_t *description, const wchar_t *icon,
                        int icon_index);


bool IsDirectoryEmpty(const std::wstring& dir_path);





bool CopyAndDeleteDirectory(const FilePath& from_path,
                            const FilePath& to_path);
#endif


bool GetTempDir(FilePath* path);

bool GetTempDir(std::wstring* path);


bool GetShmemTempDir(FilePath* path);







bool CreateTemporaryFileName(FilePath* path);

bool CreateTemporaryFileName(std::wstring* temp_file);




FILE* CreateAndOpenTemporaryFile(FilePath* path);

FILE* CreateAndOpenTemporaryShmemFile(FilePath* path);


FILE* CreateAndOpenTemporaryFileInDir(const FilePath& dir, FilePath* path);


bool CreateTemporaryFileNameInDir(const std::wstring& dir,
                                  std::wstring* temp_file);






bool CreateNewTempDirectory(const FilePath::StringType& prefix,
                            FilePath* new_temp_path);

bool CreateNewTempDirectory(const std::wstring& prefix,
                            std::wstring* new_temp_path);




bool CreateDirectory(const FilePath& full_path);

bool CreateDirectory(const std::wstring& full_path);


bool GetFileSize(const FilePath& file_path, int64* file_size);

bool GetFileSize(const std::wstring& file_path, int64* file_size);


struct FileInfo {
  
  int64 size;

  
  bool is_directory;

  
};


bool GetFileInfo(const FilePath& file_path, FileInfo* info);

bool GetFileInfo(const std::wstring& file_path, FileInfo* info);


FILE* OpenFile(const FilePath& filename, const char* mode);

FILE* OpenFile(const std::string& filename, const char* mode);
FILE* OpenFile(const std::wstring& filename, const char* mode);


bool CloseFile(FILE* file);



bool TruncateFile(FILE* file);



int ReadFile(const FilePath& filename, char* data, int size);

int ReadFile(const std::wstring& filename, char* data, int size);



int WriteFile(const FilePath& filename, const char* data, int size);

int WriteFile(const std::wstring& filename, const char* data, int size);


bool GetCurrentDirectory(FilePath* path);

bool GetCurrentDirectory(std::wstring* path);


bool SetCurrentDirectory(const FilePath& path);

bool SetCurrentDirectory(const std::wstring& current_directory);


class ScopedFILEClose {
 public:
  inline void operator()(FILE* x) const {
    if (x) {
      fclose(x);
    }
  }
};

typedef scoped_ptr_malloc<FILE, ScopedFILEClose> ScopedFILE;






class FileEnumerator {
 public:
#if defined(OS_WIN)
  typedef WIN32_FIND_DATA FindInfo;
#elif defined(OS_POSIX)
  typedef struct {
    struct stat stat;
    std::string filename;
  } FindInfo;
#endif

  enum FILE_TYPE {
    FILES                 = 0x1,
    DIRECTORIES           = 0x2,
    FILES_AND_DIRECTORIES = 0x3
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FileEnumerator(const FilePath& root_path,
                 bool recursive,
                 FileEnumerator::FILE_TYPE file_type);
  FileEnumerator(const FilePath& root_path,
                 bool recursive,
                 FileEnumerator::FILE_TYPE file_type,
                 const FilePath::StringType& pattern);
  ~FileEnumerator();

  
  FilePath Next();

  
  void GetFindInfo(FindInfo* info);

 private:
  FilePath root_path_;
  bool recursive_;
  FILE_TYPE file_type_;
  FilePath pattern_;  

  
  
  bool is_in_find_op_;

  
  
  std::stack<FilePath> pending_paths_;

#if defined(OS_WIN)
  WIN32_FIND_DATA find_data_;
  HANDLE find_handle_;
#elif defined(OS_POSIX)
  FTS* fts_;
  FTSENT* fts_ent_;
#endif

  DISALLOW_EVIL_CONSTRUCTORS(FileEnumerator);
};

class MemoryMappedFile {
 public:
  
  MemoryMappedFile();
  ~MemoryMappedFile();

  
  
  
  
  
  bool Initialize(const FilePath& file_name);

  const uint8* data() const { return data_; }
  size_t length() const { return length_; }

  
  bool IsValid();

 private:
  
  
  bool MapFileToMemory(const FilePath& file_name);

  
  void CloseHandles();

#if defined(OS_WIN)
  HANDLE file_;
  HANDLE file_mapping_;
#elif defined(OS_POSIX)
  
  int file_;
#endif
  uint8* data_;
  size_t length_;

  DISALLOW_COPY_AND_ASSIGN(MemoryMappedFile);
};



bool RenameFileAndResetSecurityDescriptor(
    const FilePath& source_file_path,
    const FilePath& target_file_path);

}  

#endif  
