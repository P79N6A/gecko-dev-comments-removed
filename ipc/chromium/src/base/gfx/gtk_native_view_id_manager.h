



#ifndef BASE_GFX_GTK_NATIVE_VIEW_ID_MANAGER_H_
#define BASE_GFX_GTK_NATIVE_VIEW_ID_MANAGER_H_

#include <map>

#include "base/singleton.h"
#include "base/gfx/native_widget_types.h"

typedef unsigned long XID;


























class GtkNativeViewManager {
 public:
  
  
  
  
  
  
  
  gfx::NativeViewId GetIdForWidget(gfx::NativeView widget);

  
  
  
  
  
  
  
  
  bool GetXIDForId(XID* xid, gfx::NativeViewId id);

  
  void OnRealize(gfx::NativeView widget);
  void OnUnrealize(gfx::NativeView widget);
  void OnDestroy(gfx::NativeView widget);

 private:
  
  GtkNativeViewManager();
  friend struct DefaultSingletonTraits<GtkNativeViewManager>;

  struct NativeViewInfo {
    NativeViewInfo()
        : x_window_id(0) {
    }

    XID x_window_id;
  };

  gfx::NativeViewId GetWidgetId(gfx::NativeView id);

  
  Lock lock_;
    
    
    std::map<gfx::NativeView, gfx::NativeViewId> native_view_to_id_;
    std::map<gfx::NativeViewId, NativeViewInfo> id_to_info_;

  DISALLOW_COPY_AND_ASSIGN(GtkNativeViewManager);
};

#endif  
