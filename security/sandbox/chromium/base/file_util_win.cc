



#include "base/file_util.h"

#include <windows.h>
#include <io.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <time.h>

#include <algorithm>
#include <limits>
#include <string>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/process/process_handle.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"

namespace base {

namespace {

const DWORD kFileShareAll =
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

}  

FilePath MakeAbsoluteFilePath(const FilePath& input) {
  ThreadRestrictions::AssertIOAllowed();
  wchar_t file_path[MAX_PATH];
  if (!_wfullpath(file_path, input.value().c_str(), MAX_PATH))
    return FilePath();
  return FilePath(file_path);
}

bool DeleteFile(const FilePath& path, bool recursive) {
  ThreadRestrictions::AssertIOAllowed();

  if (path.value().length() >= MAX_PATH)
    return false;

  
  
  if (path.empty())
    return true;

  if (!recursive) {
    
    
    File::Info file_info;
    if (GetFileInfo(path, &file_info) && file_info.is_directory)
      return RemoveDirectory(path.value().c_str()) != 0;

    
    
    
    if (::DeleteFile(path.value().c_str()) != 0)
      return true;
  }

  
  
  
  wchar_t double_terminated_path[MAX_PATH + 1] = {0};
#pragma warning(suppress:4996)  // don't complain about wcscpy deprecation
  wcscpy(double_terminated_path, path.value().c_str());

  SHFILEOPSTRUCT file_operation = {0};
  file_operation.wFunc = FO_DELETE;
  file_operation.pFrom = double_terminated_path;
  file_operation.fFlags = FOF_NOERRORUI | FOF_SILENT | FOF_NOCONFIRMATION;
  if (!recursive)
    file_operation.fFlags |= FOF_NORECURSION | FOF_FILESONLY;
  int err = SHFileOperation(&file_operation);

  
  
  
  
  if (file_operation.fAnyOperationsAborted)
    return false;

  
  
  
  
  return (err == 0 || err == ERROR_FILE_NOT_FOUND || err == 0x402 ||
          err == 0x7C);
}

bool DeleteFileAfterReboot(const FilePath& path) {
  ThreadRestrictions::AssertIOAllowed();

  if (path.value().length() >= MAX_PATH)
    return false;

  return MoveFileEx(path.value().c_str(), NULL,
                    MOVEFILE_DELAY_UNTIL_REBOOT |
                        MOVEFILE_REPLACE_EXISTING) != FALSE;
}

bool ReplaceFile(const FilePath& from_path,
                 const FilePath& to_path,
                 File::Error* error) {
  ThreadRestrictions::AssertIOAllowed();
  
  
  if (::MoveFile(from_path.value().c_str(), to_path.value().c_str()))
    return true;
  
  
  
  
  if (::ReplaceFile(to_path.value().c_str(), from_path.value().c_str(), NULL,
                    REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL)) {
    return true;
  }
  if (error)
    *error = File::OSErrorToFileError(GetLastError());
  return false;
}

bool CopyDirectory(const FilePath& from_path, const FilePath& to_path,
                   bool recursive) {
  
  
  
  
  
  ThreadRestrictions::AssertIOAllowed();

  
  
  if (from_path.value().length() >= MAX_PATH ||
      to_path.value().length() >= MAX_PATH) {
    return false;
  }

  
  FilePath real_to_path = to_path;
  if (PathExists(real_to_path)) {
    real_to_path = MakeAbsoluteFilePath(real_to_path);
    if (real_to_path.empty())
      return false;
  } else {
    real_to_path = MakeAbsoluteFilePath(real_to_path.DirName());
    if (real_to_path.empty())
      return false;
  }
  FilePath real_from_path = MakeAbsoluteFilePath(from_path);
  if (real_from_path.empty())
    return false;
  if (real_to_path.value().size() >= real_from_path.value().size() &&
      real_to_path.value().compare(0, real_from_path.value().size(),
                                   real_from_path.value()) == 0) {
    return false;
  }

  int traverse_type = FileEnumerator::FILES;
  if (recursive)
    traverse_type |= FileEnumerator::DIRECTORIES;
  FileEnumerator traversal(from_path, recursive, traverse_type);

  if (!PathExists(from_path)) {
    DLOG(ERROR) << "CopyDirectory() couldn't stat source directory: "
                << from_path.value().c_str();
    return false;
  }
  
  DCHECK(recursive || DirectoryExists(from_path));

  FilePath current = from_path;
  bool from_is_dir = DirectoryExists(from_path);
  bool success = true;
  FilePath from_path_base = from_path;
  if (recursive && DirectoryExists(to_path)) {
    
    
    from_path_base = from_path.DirName();
  }

  while (success && !current.empty()) {
    
    
    FilePath target_path(to_path);
    if (from_path_base != current) {
      if (!from_path_base.AppendRelativePath(current, &target_path)) {
        success = false;
        break;
      }
    }

    if (from_is_dir) {
      if (!DirectoryExists(target_path) &&
          !::CreateDirectory(target_path.value().c_str(), NULL)) {
        DLOG(ERROR) << "CopyDirectory() couldn't create directory: "
                    << target_path.value().c_str();
        success = false;
      }
    } else if (!internal::CopyFileUnsafe(current, target_path)) {
      DLOG(ERROR) << "CopyDirectory() couldn't create file: "
                  << target_path.value().c_str();
      success = false;
    }

    current = traversal.Next();
    if (!current.empty())
      from_is_dir = traversal.GetInfo().IsDirectory();
  }

  return success;
}

bool PathExists(const FilePath& path) {
  ThreadRestrictions::AssertIOAllowed();
  return (GetFileAttributes(path.value().c_str()) != INVALID_FILE_ATTRIBUTES);
}

bool PathIsWritable(const FilePath& path) {
  ThreadRestrictions::AssertIOAllowed();
  HANDLE dir =
      CreateFile(path.value().c_str(), FILE_ADD_FILE, kFileShareAll,
                 NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

  if (dir == INVALID_HANDLE_VALUE)
    return false;

  CloseHandle(dir);
  return true;
}

bool DirectoryExists(const FilePath& path) {
  ThreadRestrictions::AssertIOAllowed();
  DWORD fileattr = GetFileAttributes(path.value().c_str());
  if (fileattr != INVALID_FILE_ATTRIBUTES)
    return (fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0;
  return false;
}

bool GetTempDir(FilePath* path) {
  wchar_t temp_path[MAX_PATH + 1];
  DWORD path_len = ::GetTempPath(MAX_PATH, temp_path);
  if (path_len >= MAX_PATH || path_len <= 0)
    return false;
  
  
  
  *path = FilePath(temp_path).StripTrailingSeparators();
  return true;
}

FilePath GetHomeDir() {
  char16 result[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT,
                                result)) &&
      result[0]) {
    return FilePath(result);
  }

  
  FilePath temp;
  if (GetTempDir(&temp))
    return temp;

  
  return FilePath(L"C:\\");
}

bool CreateTemporaryFile(FilePath* path) {
  ThreadRestrictions::AssertIOAllowed();

  FilePath temp_file;

  if (!GetTempDir(path))
    return false;

  if (CreateTemporaryFileInDir(*path, &temp_file)) {
    *path = temp_file;
    return true;
  }

  return false;
}





FILE* CreateAndOpenTemporaryFileInDir(const FilePath& dir, FilePath* path) {
  ThreadRestrictions::AssertIOAllowed();
  if (!CreateTemporaryFileInDir(dir, path)) {
    return NULL;
  }
  
  
  
  return OpenFile(*path, "wb+");
}

bool CreateTemporaryFileInDir(const FilePath& dir, FilePath* temp_file) {
  ThreadRestrictions::AssertIOAllowed();

  wchar_t temp_name[MAX_PATH + 1];

  if (!GetTempFileName(dir.value().c_str(), L"", 0, temp_name)) {
    DPLOG(WARNING) << "Failed to get temporary file name in "
                   << UTF16ToUTF8(dir.value());
    return false;
  }

  wchar_t long_temp_name[MAX_PATH + 1];
  DWORD long_name_len = GetLongPathName(temp_name, long_temp_name, MAX_PATH);
  if (long_name_len > MAX_PATH || long_name_len == 0) {
    
    *temp_file = FilePath(temp_name);
    return true;
  }

  FilePath::StringType long_temp_name_str;
  long_temp_name_str.assign(long_temp_name, long_name_len);
  *temp_file = FilePath(long_temp_name_str);
  return true;
}

bool CreateTemporaryDirInDir(const FilePath& base_dir,
                             const FilePath::StringType& prefix,
                             FilePath* new_dir) {
  ThreadRestrictions::AssertIOAllowed();

  FilePath path_to_create;

  for (int count = 0; count < 50; ++count) {
    
    
    string16 new_dir_name;
    new_dir_name.assign(prefix);
    new_dir_name.append(IntToString16(GetCurrentProcId()));
    new_dir_name.push_back('_');
    new_dir_name.append(IntToString16(RandInt(0, kint16max)));

    path_to_create = base_dir.Append(new_dir_name);
    if (::CreateDirectory(path_to_create.value().c_str(), NULL)) {
      *new_dir = path_to_create;
      return true;
    }
  }

  return false;
}

bool CreateNewTempDirectory(const FilePath::StringType& prefix,
                            FilePath* new_temp_path) {
  ThreadRestrictions::AssertIOAllowed();

  FilePath system_temp_dir;
  if (!GetTempDir(&system_temp_dir))
    return false;

  return CreateTemporaryDirInDir(system_temp_dir, prefix, new_temp_path);
}

bool CreateDirectoryAndGetError(const FilePath& full_path,
                                File::Error* error) {
  ThreadRestrictions::AssertIOAllowed();

  
  const wchar_t* full_path_str = full_path.value().c_str();
  DWORD fileattr = ::GetFileAttributes(full_path_str);
  if (fileattr != INVALID_FILE_ATTRIBUTES) {
    if ((fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      DVLOG(1) << "CreateDirectory(" << full_path_str << "), "
               << "directory already exists.";
      return true;
    }
    DLOG(WARNING) << "CreateDirectory(" << full_path_str << "), "
                  << "conflicts with existing file.";
    if (error) {
      *error = File::FILE_ERROR_NOT_A_DIRECTORY;
    }
    return false;
  }

  

  
  
  
  FilePath parent_path(full_path.DirName());
  if (parent_path.value() == full_path.value()) {
    if (error) {
      *error = File::FILE_ERROR_NOT_FOUND;
    }
    return false;
  }
  if (!CreateDirectoryAndGetError(parent_path, error)) {
    DLOG(WARNING) << "Failed to create one of the parent directories.";
    if (error) {
      DCHECK(*error != File::FILE_OK);
    }
    return false;
  }

  if (!::CreateDirectory(full_path_str, NULL)) {
    DWORD error_code = ::GetLastError();
    if (error_code == ERROR_ALREADY_EXISTS && DirectoryExists(full_path)) {
      
      
      
      
      return true;
    } else {
      if (error)
        *error = File::OSErrorToFileError(error_code);
      DLOG(WARNING) << "Failed to create directory " << full_path_str
                    << ", last error is " << error_code << ".";
      return false;
    }
  } else {
    return true;
  }
}

bool NormalizeFilePath(const FilePath& path, FilePath* real_path) {
  ThreadRestrictions::AssertIOAllowed();
  FilePath mapped_file;
  if (!NormalizeToNativeFilePath(path, &mapped_file))
    return false;
  
  
  
  
  return DevicePathToDriveLetterPath(mapped_file, real_path);
}

bool DevicePathToDriveLetterPath(const FilePath& nt_device_path,
                                 FilePath* out_drive_letter_path) {
  ThreadRestrictions::AssertIOAllowed();

  
  const int kDriveMappingSize = 1024;
  wchar_t drive_mapping[kDriveMappingSize] = {'\0'};
  if (!::GetLogicalDriveStrings(kDriveMappingSize - 1, drive_mapping)) {
    DLOG(ERROR) << "Failed to get drive mapping.";
    return false;
  }

  
  
  wchar_t* drive_map_ptr = drive_mapping;
  wchar_t device_path_as_string[MAX_PATH];
  wchar_t drive[] = L" :";

  
  
  
  while (*drive_map_ptr) {
    drive[0] = drive_map_ptr[0];  

    if (QueryDosDevice(drive, device_path_as_string, MAX_PATH)) {
      FilePath device_path(device_path_as_string);
      if (device_path == nt_device_path ||
          device_path.IsParent(nt_device_path)) {
        *out_drive_letter_path = FilePath(drive +
            nt_device_path.value().substr(wcslen(device_path_as_string)));
        return true;
      }
    }
    
    
    while (*drive_map_ptr++);
  }

  
  
  
  return false;
}

bool NormalizeToNativeFilePath(const FilePath& path, FilePath* nt_path) {
  ThreadRestrictions::AssertIOAllowed();
  
  
  
  
  
  base::win::ScopedHandle file_handle(
      ::CreateFile(path.value().c_str(),
                   GENERIC_READ,
                   kFileShareAll,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL));
  if (!file_handle)
    return false;

  
  
  
  base::win::ScopedHandle file_map_handle(
      ::CreateFileMapping(file_handle.Get(),
                          NULL,
                          PAGE_READONLY,
                          0,
                          1,  
                          NULL));
  if (!file_map_handle)
    return false;

  
  void* file_view = MapViewOfFile(file_map_handle.Get(),
                                  FILE_MAP_READ, 0, 0, 1);
  if (!file_view)
    return false;

  
  
  
  
  
  
  const int kMaxPathLength = MAX_PATH + 10;
  wchar_t mapped_file_path[kMaxPathLength];
  bool success = false;
  HANDLE cp = GetCurrentProcess();
  if (::GetMappedFileNameW(cp, file_view, mapped_file_path, kMaxPathLength)) {
    *nt_path = FilePath(mapped_file_path);
    success = true;
  }
  ::UnmapViewOfFile(file_view);
  return success;
}



bool IsLink(const FilePath& file_path) {
  return false;
}

bool GetFileInfo(const FilePath& file_path, File::Info* results) {
  ThreadRestrictions::AssertIOAllowed();

  WIN32_FILE_ATTRIBUTE_DATA attr;
  if (!GetFileAttributesEx(file_path.value().c_str(),
                           GetFileExInfoStandard, &attr)) {
    return false;
  }

  ULARGE_INTEGER size;
  size.HighPart = attr.nFileSizeHigh;
  size.LowPart = attr.nFileSizeLow;
  results->size = size.QuadPart;

  results->is_directory =
      (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  results->last_modified = Time::FromFileTime(attr.ftLastWriteTime);
  results->last_accessed = Time::FromFileTime(attr.ftLastAccessTime);
  results->creation_time = Time::FromFileTime(attr.ftCreationTime);

  return true;
}

FILE* OpenFile(const FilePath& filename, const char* mode) {
  ThreadRestrictions::AssertIOAllowed();
  std::wstring w_mode = ASCIIToWide(std::string(mode));
  return _wfsopen(filename.value().c_str(), w_mode.c_str(), _SH_DENYNO);
}

FILE* FileToFILE(File file, const char* mode) {
  if (!file.IsValid())
    return NULL;
  int fd =
      _open_osfhandle(reinterpret_cast<intptr_t>(file.GetPlatformFile()), 0);
  if (fd < 0)
    return NULL;
  file.TakePlatformFile();
  FILE* stream = _fdopen(fd, mode);
  if (!stream)
    _close(fd);
  return stream;
}

int ReadFile(const FilePath& filename, char* data, int max_size) {
  ThreadRestrictions::AssertIOAllowed();
  base::win::ScopedHandle file(CreateFile(filename.value().c_str(),
                                          GENERIC_READ,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          FILE_FLAG_SEQUENTIAL_SCAN,
                                          NULL));
  if (!file)
    return -1;

  DWORD read;
  if (::ReadFile(file, data, max_size, &read, NULL))
    return read;

  return -1;
}

int WriteFile(const FilePath& filename, const char* data, int size) {
  ThreadRestrictions::AssertIOAllowed();
  base::win::ScopedHandle file(CreateFile(filename.value().c_str(),
                                          GENERIC_WRITE,
                                          0,
                                          NULL,
                                          CREATE_ALWAYS,
                                          0,
                                          NULL));
  if (!file) {
    DPLOG(WARNING) << "CreateFile failed for path "
                   << UTF16ToUTF8(filename.value());
    return -1;
  }

  DWORD written;
  BOOL result = ::WriteFile(file, data, size, &written, NULL);
  if (result && static_cast<int>(written) == size)
    return written;

  if (!result) {
    
    DPLOG(WARNING) << "writing file " << UTF16ToUTF8(filename.value())
                   << " failed";
  } else {
    
    DLOG(WARNING) << "wrote" << written << " bytes to "
                  << UTF16ToUTF8(filename.value()) << " expected " << size;
  }
  return -1;
}

int AppendToFile(const FilePath& filename, const char* data, int size) {
  ThreadRestrictions::AssertIOAllowed();
  base::win::ScopedHandle file(CreateFile(filename.value().c_str(),
                                          FILE_APPEND_DATA,
                                          0,
                                          NULL,
                                          OPEN_EXISTING,
                                          0,
                                          NULL));
  if (!file) {
    DPLOG(WARNING) << "CreateFile failed for path "
                   << UTF16ToUTF8(filename.value());
    return -1;
  }

  DWORD written;
  BOOL result = ::WriteFile(file, data, size, &written, NULL);
  if (result && static_cast<int>(written) == size)
    return written;

  if (!result) {
    
    DPLOG(WARNING) << "writing file " << UTF16ToUTF8(filename.value())
                   << " failed";
  } else {
    
    DLOG(WARNING) << "wrote" << written << " bytes to "
                  << UTF16ToUTF8(filename.value()) << " expected " << size;
  }
  return -1;
}


bool GetCurrentDirectory(FilePath* dir) {
  ThreadRestrictions::AssertIOAllowed();

  wchar_t system_buffer[MAX_PATH];
  system_buffer[0] = 0;
  DWORD len = ::GetCurrentDirectory(MAX_PATH, system_buffer);
  if (len == 0 || len > MAX_PATH)
    return false;
  
  
  
  std::wstring dir_str(system_buffer);
  *dir = FilePath(dir_str).StripTrailingSeparators();
  return true;
}


bool SetCurrentDirectory(const FilePath& directory) {
  ThreadRestrictions::AssertIOAllowed();
  BOOL ret = ::SetCurrentDirectory(directory.value().c_str());
  return ret != 0;
}

int GetMaximumPathComponentLength(const FilePath& path) {
  ThreadRestrictions::AssertIOAllowed();

  wchar_t volume_path[MAX_PATH];
  if (!GetVolumePathNameW(path.NormalizePathSeparators().value().c_str(),
                          volume_path,
                          arraysize(volume_path))) {
    return -1;
  }

  DWORD max_length = 0;
  if (!GetVolumeInformationW(volume_path, NULL, 0, NULL, &max_length, NULL,
                             NULL, 0)) {
    return -1;
  }

  
  size_t prefix = path.StripTrailingSeparators().value().size() + 1;
  
  
  int whole_path_limit = std::max(0, MAX_PATH - 1 - static_cast<int>(prefix));
  return std::min(whole_path_limit, static_cast<int>(max_length));
}



namespace internal {

bool MoveUnsafe(const FilePath& from_path, const FilePath& to_path) {
  ThreadRestrictions::AssertIOAllowed();

  
  
  if (from_path.value().length() >= MAX_PATH ||
      to_path.value().length() >= MAX_PATH) {
    return false;
  }
  if (MoveFileEx(from_path.value().c_str(), to_path.value().c_str(),
                 MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) != 0)
    return true;

  
  
  bool ret = false;
  DWORD last_error = ::GetLastError();

  if (DirectoryExists(from_path)) {
    
    
    
    ret = internal::CopyAndDeleteDirectory(from_path, to_path);
  }

  if (!ret) {
    
    
    ::SetLastError(last_error);
  }

  return ret;
}

bool CopyFileUnsafe(const FilePath& from_path, const FilePath& to_path) {
  ThreadRestrictions::AssertIOAllowed();

  
  
  if (from_path.value().length() >= MAX_PATH ||
      to_path.value().length() >= MAX_PATH) {
    return false;
  }

  
  
  
  
  const wchar_t* dest = to_path.value().c_str();
  if (!::CopyFile(from_path.value().c_str(), dest, false)) {
    
    return false;
  }
  DWORD attrs = GetFileAttributes(dest);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  if (attrs & FILE_ATTRIBUTE_READONLY) {
    SetFileAttributes(dest, attrs & ~FILE_ATTRIBUTE_READONLY);
  }
  return true;
}

bool CopyAndDeleteDirectory(const FilePath& from_path,
                            const FilePath& to_path) {
  ThreadRestrictions::AssertIOAllowed();
  if (CopyDirectory(from_path, to_path, true)) {
    if (DeleteFile(from_path, true))
      return true;

    
    
    
    
  }
  return false;
}

}  
}  
