



#ifndef CHROME_COMMON_WIN_UTIL_H_
#define CHROME_COMMON_WIN_UTIL_H_

#include <objbase.h>

#include <string>
#include <vector>

#include "app/gfx/chrome_font.h"
#include "base/fix_wp64.h"
#include "base/gfx/rect.h"
#include "base/scoped_handle.h"

class FilePath;

namespace win_util {



using ::ScopedHandle;
using ::ScopedFindFileHandle;
using ::ScopedHDC;
using ::ScopedBitmap;
using ::ScopedHRGN;







template<typename T>
class CoMemReleaser {
 public:
  explicit CoMemReleaser() : mem_ptr_(NULL) {}

  ~CoMemReleaser() {
    if (mem_ptr_)
      CoTaskMemFree(mem_ptr_);
  }

  T** operator&() {
    return &mem_ptr_;
  }

  operator T*() {
    return mem_ptr_;
  }

 private:
  T* mem_ptr_;

  DISALLOW_COPY_AND_ASSIGN(CoMemReleaser);
};



class ScopedCOMInitializer {
 public:
  ScopedCOMInitializer() : hr_(CoInitialize(NULL)) {
  }

  ScopedCOMInitializer::~ScopedCOMInitializer() {
    if (SUCCEEDED(hr_))
      CoUninitialize();
  }

  
  
  inline HRESULT error_code() const {
    return hr_;
  }

 protected:
  HRESULT hr_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedCOMInitializer);
};





std::wstring FormatSystemTime(const SYSTEMTIME& time,
                              const std::wstring& format);





std::wstring FormatSystemDate(const SYSTEMTIME& date,
                              const std::wstring& format);





bool ConvertToLongPath(const std::wstring& short_path, std::wstring* long_path);



bool IsDoubleClick(const POINT& origin,
                   const POINT& current,
                   DWORD elapsed_time);



bool IsDrag(const POINT& origin, const POINT& current);


bool ShouldUseVistaFrame();






bool OpenItemViaShell(const FilePath& full_path, bool ask_for_app);




bool OpenItemViaShellNoZoneCheck(const FilePath& full_path,
                                 bool ask_for_app);




bool OpenItemWithExternalApp(const std::wstring& full_path);

















std::wstring FormatFilterForExtensions(
    const std::vector<std::wstring>& file_ext,
    const std::vector<std::wstring>& ext_desc,
    bool include_all_files);
















bool SaveFileAs(HWND owner,
                const std::wstring& suggested_name,
                std::wstring* final_name);
















bool SaveFileAsWithFilter(HWND owner,
                          const std::wstring& suggested_name,
                          const std::wstring& filter,
                          const std::wstring& def_ext,
                          bool ignore_suggested_ext,
                          unsigned* index,
                          std::wstring* final_name);











std::wstring AppendExtensionIfNeeded(const std::wstring& filename,
                                     const std::wstring& filter_selected,
                                     const std::wstring& suggested_ext);



void AdjustWindowToFit(HWND hwnd);




void CenterAndSizeWindow(HWND parent, HWND window, const SIZE& pref,
                         bool pref_is_client);



bool EdgeHasTopmostAutoHideTaskbar(UINT edge, HMONITOR monitor);



HANDLE GetSectionFromProcess(HANDLE section, HANDLE process, bool read_only);



bool DoesWindowBelongToActiveWindow(HWND window);



void EnsureRectIsVisibleInRect(const gfx::Rect& parent_rect,
                               gfx::Rect* child_rect,
                               int padding);







void SetChildBounds(HWND child_window, HWND parent_window,
                    HWND insert_after_window, const gfx::Rect& bounds,
                    int padding, unsigned long flags);



gfx::Rect GetMonitorBoundsForRect(const gfx::Rect& rect);





bool IsNumPadDigit(int key_code, bool extended_key);



void GrabWindowSnapshot(HWND window_handle,
                        std::vector<unsigned char>* png_representation);


bool IsWindowActive(HWND hwnd);




bool IsReservedName(const std::wstring& filename);



bool IsShellIntegratedExtension(const std::wstring& eextension);





int MessageBox(HWND hwnd,
               const std::wstring& text,
               const std::wstring& caption,
               UINT flags);


ChromeFont GetWindowTitleFont();


extern const int kAutoHideTaskbarThicknessPx;

}  

#endif  
