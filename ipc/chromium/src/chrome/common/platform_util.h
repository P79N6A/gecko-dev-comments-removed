



#ifndef CHROME_COMMON_PLATFORM_UTIL_H_
#define CHROME_COMMON_PLATFORM_UTIL_H_

#include "base/gfx/native_widget_types.h"
#include "base/string16.h"

class FilePath;

namespace platform_util {


void ShowItemInFolder(const FilePath& full_path);


gfx::NativeWindow GetTopLevel(gfx::NativeView view);


string16 GetWindowTitle(gfx::NativeWindow window);


bool IsWindowActive(gfx::NativeWindow window);

}

#endif  
