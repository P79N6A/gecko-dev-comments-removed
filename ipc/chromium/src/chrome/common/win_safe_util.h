



#ifndef CHROME_COMMON_WIN_SAFE_UTIL_H__
#define CHROME_COMMON_WIN_SAFE_UTIL_H__

#include <string>
#include <windows.h>

class FilePath;

namespace win_util {

























bool SaferOpenItemViaShell(HWND hwnd, const std::wstring& window_title,
                           const FilePath& full_path,
                           const std::wstring& source_url, bool ask_for_app);





bool SetInternetZoneIdentifier(const FilePath& full_path);

}  

#endif 
