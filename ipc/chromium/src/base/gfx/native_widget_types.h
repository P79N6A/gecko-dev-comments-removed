



#ifndef BASE_GFX_NATIVE_WIDGET_TYPES_H_
#define BASE_GFX_NATIVE_WIDGET_TYPES_H_

#include "base/basictypes.h"
#include "build/build_config.h"


























#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_MACOSX)
struct CGContext;
#ifdef __OBJC__
@class NSView;
@class NSWindow;
@class NSTextField;
#else
class NSView;
class NSWindow;
class NSTextField;
#endif  
#elif defined(OS_LINUX)
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;
typedef struct _cairo_surface cairo_surface_t;
#endif

namespace gfx {

#if defined(OS_WIN)
typedef HWND NativeView;
typedef HWND NativeWindow;
typedef HWND NativeEditView;
typedef HDC NativeDrawingContext;
#elif defined(OS_MACOSX)
typedef NSView* NativeView;
typedef NSWindow* NativeWindow;
typedef NSTextField* NativeEditView;
typedef CGContext* NativeDrawingContext;
#elif defined(OS_LINUX)
typedef GtkWidget* NativeView;
typedef GtkWindow* NativeWindow;
typedef GtkWidget* NativeEditView;
typedef cairo_surface_t* NativeDrawingContext;
#endif






typedef intptr_t NativeViewId;





#if defined(OS_WIN) || defined(OS_MACOSX)
static inline NativeView NativeViewFromId(NativeViewId id) {
  return reinterpret_cast<NativeView>(id);
}
#elif defined(OS_LINUX)





#define NativeViewFromId(x) NATIVE_VIEW_FROM_ID_NOT_AVAILIBLE_ON_LINUX

#endif  



#if defined(OS_WIN) || defined(OS_MACOSX)
static inline NativeViewId IdFromNativeView(NativeView view) {
  return reinterpret_cast<NativeViewId>(view);
}
#elif defined(OS_LINUX)

NativeViewId IdFromNativeView(NativeView view);
#endif  

}  

#endif  
