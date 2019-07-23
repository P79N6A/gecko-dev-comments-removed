



#include "chrome/common/platform_util.h"

#include <atlbase.h>
#include <atlapp.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/gfx/native_widget_types.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "chrome/common/win_util.h"

namespace platform_util {

void ShowItemInFolder(const FilePath& full_path) {
  FilePath dir = full_path.DirName();
  
  if (dir.value() == L"" || !file_util::EnsureEndsWithSeparator(&dir))
    return;

  typedef HRESULT (WINAPI *SHOpenFolderAndSelectItemsFuncPtr)(
      PCIDLIST_ABSOLUTE pidl_Folder,
      UINT cidl,
      PCUITEMID_CHILD_ARRAY pidls,
      DWORD flags);

  static SHOpenFolderAndSelectItemsFuncPtr open_folder_and_select_itemsPtr =
    NULL;
  static bool initialize_open_folder_proc = true;
  if (initialize_open_folder_proc) {
    initialize_open_folder_proc = false;
    
    
    
    
    
    HMODULE shell32_base = GetModuleHandle(L"shell32.dll");
    if (!shell32_base) {
      NOTREACHED();
      return;
    }
    open_folder_and_select_itemsPtr =
        reinterpret_cast<SHOpenFolderAndSelectItemsFuncPtr>
            (GetProcAddress(shell32_base, "SHOpenFolderAndSelectItems"));
  }
  if (!open_folder_and_select_itemsPtr) {
    ShellExecute(NULL, _T("open"), dir.value().c_str(), NULL, NULL, SW_SHOW);
    return;
  }

  CComPtr<IShellFolder> desktop;
  HRESULT hr = SHGetDesktopFolder(&desktop);
  if (FAILED(hr))
    return;

  win_util::CoMemReleaser<ITEMIDLIST> dir_item;
  hr = desktop->ParseDisplayName(NULL, NULL,
                                 const_cast<wchar_t *>(dir.value().c_str()),
                                 NULL, &dir_item, NULL);
  if (FAILED(hr))
    return;

  win_util::CoMemReleaser<ITEMIDLIST> file_item;
  hr = desktop->ParseDisplayName(NULL, NULL,
      const_cast<wchar_t *>(full_path.value().c_str()),
      NULL, &file_item, NULL);
  if (FAILED(hr))
    return;

  const ITEMIDLIST* highlight[] = {
    {file_item},
  };
  (*open_folder_and_select_itemsPtr)(dir_item, arraysize(highlight),
                                     highlight, NULL);
}

gfx::NativeWindow GetTopLevel(gfx::NativeView view) {
  return GetAncestor(view, GA_ROOT);
}

string16 GetWindowTitle(gfx::NativeWindow window_handle) {
  std::wstring result;
  int length = ::GetWindowTextLength(window_handle) + 1;
  ::GetWindowText(window_handle, WriteInto(&result, length), length);
  return WideToUTF16(result);
}

bool IsWindowActive(gfx::NativeWindow window) {
  return ::GetForegroundWindow() == window;
}

}  
