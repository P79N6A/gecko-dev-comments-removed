






#ifndef BASE_FILE_UTIL_H_
#define BASE_FILE_UTIL_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(ANDROID)
#include <sys/stat.h>
#elif defined(OS_POSIX) 
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <stdio.h>

#include <stack>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/file_path.h"

namespace file_util {





bool EndsWithSeparator(const FilePath& path);

bool EndsWithSeparator(std::wstring* path);
bool EndsWithSeparator(const std::wstring& path);




void TrimTrailingSeparator(std::wstring* dir);





void UpOneDirectory(std::wstring* dir);



std::wstring GetFilenameFromPath(const std::wstring& path);


FilePath::StringType GetFileExtensionFromPath(const FilePath& path);

std::wstring GetFileExtensionFromPath(const std::wstring& path);


void AppendToPath(std::wstring* path, const std::wstring& new_ending);



bool AbsolutePath(FilePath* path);

bool AbsolutePath(std::wstring* path);


void InsertBeforeExtension(FilePath* path, const FilePath::StringType& suffix);


void ReplaceExtension(FilePath* file_name,
                      const FilePath::StringType& extension);

#if defined(OS_WIN)

void InsertBeforeExtension(std::wstring* path, const std::wstring& suffix);
void ReplaceExtension(std::wstring* file_name, const std::wstring& extension);
#endif








bool Delete(const FilePath& path);

bool Delete(const std::wstring& path);


bool CopyFile(const FilePath& from_path, const FilePath& to_path);

bool CopyFile(const std::wstring& from_path, const std::wstring& to_path);



bool PathExists(const FilePath& path);

bool PathExists(const std::wstring& path);


bool PathIsWritable(const FilePath& path);

bool PathIsWritable(const std::wstring& path);


bool DirectoryExists(const FilePath& path);

bool DirectoryExists(const std::wstring& path);

#if defined(OS_POSIX)



bool ReadFromFD(int fd, char* buffer, size_t bytes);
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


bool GetFileSize(const FilePath& file_path, int64_t* file_size);

bool GetFileSize(const std::wstring& file_path, int64_t* file_size);


struct FileInfo {
  
  int64_t size;

  
  bool is_directory;

  
};


bool GetFileInfo(const FilePath& file_path, FileInfo* info);

bool GetFileInfo(const std::wstring& file_path, FileInfo* info);


FILE* OpenFile(const FilePath& filename, const char* mode);

FILE* OpenFile(const std::string& filename, const char* mode);
FILE* OpenFile(const std::wstring& filename, const char* mode);


bool CloseFile(FILE* file);



int ReadFile(const FilePath& filename, char* data, int size);

int ReadFile(const std::wstring& filename, char* data, int size);



int WriteFile(const FilePath& filename, const char* data, int size);

int WriteFile(const std::wstring& filename, const char* data, int size);


bool GetCurrentDirectory(FilePath* path);

bool GetCurrentDirectory(std::wstring* path);


bool SetCurrentDirectory(const FilePath& path);

bool SetCurrentDirectory(const std::wstring& current_directory);

}  

#endif  
