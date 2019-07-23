



#ifndef BASE_GFX_GTK_UTIL_H_
#define BASE_GFX_GTK_UTIL_H_

#include <stdint.h>
#include <vector>

#include <glib-object.h>

#include "base/scoped_ptr.h"

typedef struct _GdkColor GdkColor;
typedef struct _GdkRegion GdkRegion;




#define GDK_COLOR_RGB(r, g, b) {0, r * 257, g * 257, b * 257}

namespace gfx {

class Rect;

extern const GdkColor kGdkWhite;
extern const GdkColor kGdkBlack;
extern const GdkColor kGdkGreen;


void SubtractRectanglesFromRegion(GdkRegion* region,
                                  const std::vector<Rect>& cutouts);

}  

namespace {


template <typename Type>
struct GObjectUnrefer {
  void operator()(Type* ptr) const {
    if (ptr)
      g_object_unref(ptr);
  }
};
}  




template<class T>
struct ScopedGObject {
  typedef scoped_ptr_malloc<T, GObjectUnrefer<T> > Type;
};

#endif  
