



#ifndef BASE_FILE_VERSION_INFO_H__
#define BASE_FILE_VERSION_INFO_H__

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif  

#include <string>

#include "base/base_export.h"
#include "base/strings/string16.h"

namespace base {
class FilePath;
}











class FileVersionInfo {
 public:
  virtual ~FileVersionInfo() {}
#if defined(OS_WIN) || defined(OS_MACOSX)
  
  
  
  BASE_EXPORT static FileVersionInfo* CreateFileVersionInfo(
      const base::FilePath& file_path);
#endif  

#if defined(OS_WIN)
  
  
  BASE_EXPORT static FileVersionInfo* CreateFileVersionInfoForModule(
      HMODULE module);

  
  
  
  
  __forceinline static FileVersionInfo*
  CreateFileVersionInfoForCurrentModule() {
    HMODULE module = reinterpret_cast<HMODULE>(&__ImageBase);
    return CreateFileVersionInfoForModule(module);
  }
#else
  
  
  BASE_EXPORT static FileVersionInfo* CreateFileVersionInfoForCurrentModule();
#endif  

  
  
  virtual string16 company_name() = 0;
  virtual string16 company_short_name() = 0;
  virtual string16 product_name() = 0;
  virtual string16 product_short_name() = 0;
  virtual string16 internal_name() = 0;
  virtual string16 product_version() = 0;
  virtual string16 private_build() = 0;
  virtual string16 special_build() = 0;
  virtual string16 comments() = 0;
  virtual string16 original_filename() = 0;
  virtual string16 file_description() = 0;
  virtual string16 file_version() = 0;
  virtual string16 legal_copyright() = 0;
  virtual string16 legal_trademarks() = 0;
  virtual string16 last_change() = 0;
  virtual bool is_official_build() = 0;
};

#endif  
