



#include "base/gfx/native_widget_types.h"

#include "base/gfx/gtk_native_view_id_manager.h"
#include "base/logging.h"

namespace gfx {

NativeViewId IdFromNativeView(NativeView view) {
  return Singleton<GtkNativeViewManager>()->GetIdForWidget(view);
}

}  
