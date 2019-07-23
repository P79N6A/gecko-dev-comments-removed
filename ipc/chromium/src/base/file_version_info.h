



#ifndef BASE_FILE_VERSION_INFO_H__
#define BASE_FILE_VERSION_INFO_H__

#include <string>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/scoped_ptr.h"

#if defined(OS_WIN)
struct tagVS_FIXEDFILEINFO;
typedef tagVS_FIXEDFILEINFO VS_FIXEDFILEINFO;
#elif defined(OS_MACOSX)
#ifdef __OBJC__
@class NSBundle;
#else
class NSBundle;
#endif
#endif





class FileVersionInfo {
 public:
  
  
  
  static FileVersionInfo* CreateFileVersionInfo(const FilePath& file_path);
  
  
  static FileVersionInfo* CreateFileVersionInfo(const std::wstring& file_path);

  
  
  static FileVersionInfo* CreateFileVersionInfoForCurrentModule();

  ~FileVersionInfo();

  
  
  std::wstring company_name();
  std::wstring company_short_name();
  std::wstring product_name();
  std::wstring product_short_name();
  std::wstring internal_name();
  std::wstring product_version();
  std::wstring private_build();
  std::wstring special_build();
  std::wstring comments();
  std::wstring original_filename();
  std::wstring file_description();
  std::wstring file_version();
  std::wstring legal_copyright();
  std::wstring legal_trademarks();
  std::wstring last_change();
  bool is_official_build();

  
  bool GetValue(const wchar_t* name, std::wstring* value);

  
  
  std::wstring GetStringValue(const wchar_t* name);

#ifdef OS_WIN
  
  VS_FIXEDFILEINFO* fixed_file_info() { return fixed_file_info_; }
#endif

 private:
#if defined(OS_WIN)
  FileVersionInfo(void* data, int language, int code_page);

  scoped_ptr_malloc<char> data_;
  int language_;
  int code_page_;
  
  VS_FIXEDFILEINFO* fixed_file_info_;
#elif defined(OS_MACOSX)
  explicit FileVersionInfo(NSBundle *bundle);

  NSBundle *bundle_;
#elif defined(OS_LINUX)
  FileVersionInfo();
#endif

  DISALLOW_EVIL_CONSTRUCTORS(FileVersionInfo);
};

#endif  
