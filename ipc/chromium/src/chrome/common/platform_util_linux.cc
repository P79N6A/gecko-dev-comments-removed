



#include "chrome/common/platform_util.h"

#include <gtk/gtk.h>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/process_util.h"
#include "base/string_util.h"

namespace platform_util {




void ShowItemInFolder(const FilePath& full_path) {
  FilePath dir = full_path.DirName();
  if (!file_util::DirectoryExists(dir))
    return;

  std::vector<std::string> argv;
  argv.push_back("xdg-open");
  argv.push_back(dir.value());
  base::file_handle_mapping_vector no_files;
  base::LaunchApp(argv, no_files, false, NULL);
}

gfx::NativeWindow GetTopLevel(gfx::NativeView view) {
  return GTK_WINDOW(gtk_widget_get_toplevel(view));
}

string16 GetWindowTitle(gfx::NativeWindow window) {
  const gchar* title = gtk_window_get_title(window);
  return UTF8ToUTF16(title);
}

bool IsWindowActive(gfx::NativeWindow window) {
  return gtk_window_is_active(window);
}

}  
