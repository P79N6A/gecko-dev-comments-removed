



#include "base/test_file_util.h"

#include <windows.h>

#include <vector>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/scoped_handle.h"

namespace file_util {



static const ptrdiff_t kPageSize = 4096;

bool EvictFileFromSystemCache(const FilePath& file) {
  
  ScopedHandle file_handle(
      CreateFile(file.value().c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                 OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL));
  if (!file_handle)
    return false;

  
  BY_HANDLE_FILE_INFORMATION bhi = {0};
  CHECK(::GetFileInformationByHandle(file_handle, &bhi));

  
  

  
  
  
  
  char buffer[2 * kPageSize - 1];
  
  char* read_write = reinterpret_cast<char*>(
      reinterpret_cast<ptrdiff_t>(buffer + kPageSize - 1) & ~(kPageSize - 1));
  DCHECK((reinterpret_cast<int>(read_write) % kPageSize) == 0);

  
  
  bool file_is_page_aligned = true;
  int total_bytes = 0;
  DWORD bytes_read, bytes_written;
  for (;;) {
    bytes_read = 0;
    ReadFile(file_handle, read_write, kPageSize, &bytes_read, NULL);
    if (bytes_read == 0)
      break;

    if (bytes_read < kPageSize) {
      
      
      
      
      ZeroMemory(read_write + bytes_read, kPageSize - bytes_read);
      file_is_page_aligned = false;
    }

    
    
    
    DCHECK((total_bytes % kPageSize) == 0);
    SetFilePointer(file_handle, total_bytes, NULL, FILE_BEGIN);
    if (!WriteFile(file_handle, read_write, kPageSize, &bytes_written, NULL) ||
        bytes_written != kPageSize) {
      DCHECK(false);
      return false;
    }

    total_bytes += bytes_read;

    
    if (!file_is_page_aligned)
      break;
  }

  if (!file_is_page_aligned) {
    
    
    
    file_handle.Set(NULL);
    file_handle.Set(CreateFile(file.value().c_str(), GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, 0, NULL));
    CHECK(SetFilePointer(file_handle, total_bytes, NULL, FILE_BEGIN) !=
          INVALID_SET_FILE_POINTER);
    CHECK(::SetEndOfFile(file_handle));
  }

  
  CHECK(::SetFileTime(file_handle, &bhi.ftCreationTime, &bhi.ftLastAccessTime,
                      &bhi.ftLastWriteTime));

  return true;
}



bool CopyRecursiveDirNoCache(const std::wstring& source_dir,
                             const std::wstring& dest_dir) {
  
  if (!CreateDirectory(dest_dir)) {
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
  }

  std::vector<std::wstring> files_copied;

  std::wstring src(source_dir);
  file_util::AppendToPath(&src, L"*");

  WIN32_FIND_DATA fd;
  HANDLE fh = FindFirstFile(src.c_str(), &fd);
  if (fh == INVALID_HANDLE_VALUE)
    return false;

  do {
    std::wstring cur_file(fd.cFileName);
    if (cur_file == L"." || cur_file == L"..")
      continue;  

    std::wstring cur_source_path(source_dir);
    file_util::AppendToPath(&cur_source_path, cur_file);

    std::wstring cur_dest_path(dest_dir);
    file_util::AppendToPath(&cur_dest_path, cur_file);

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      
      if (!CopyRecursiveDirNoCache(cur_source_path, cur_dest_path)) {
        FindClose(fh);
        return false;
      }
    } else {
      
      if (!::CopyFile(cur_source_path.c_str(), cur_dest_path.c_str(), false)) {
        FindClose(fh);
        return false;
      }

      
      
      
      
      EvictFileFromSystemCache(FilePath::FromWStringHack(cur_dest_path));
    }
  } while (FindNextFile(fh, &fd));

  FindClose(fh);
  return true;
}

}  
