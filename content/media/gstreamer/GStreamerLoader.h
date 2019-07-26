




#ifndef GStreamerLoader_h_
#define GStreamerLoader_h_

#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/gstelementfactory.h>
#include <gst/gststructure.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wreserved-user-defined-literal"
#include <gst/video/video.h>
#pragma GCC diagnostic pop

#if GST_VERSION_MAJOR == 1
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>
#endif

namespace mozilla {





bool load_gstreamer();





#define GST_FUNC(_, func) extern typeof(::func)* func;
#define REPLACE_FUNC(func) GST_FUNC(-1, func)
#include "GStreamerFunctionList.h"
#undef GST_FUNC
#undef REPLACE_FUNC

}

#undef GST_CAPS_ANY
#define GST_CAPS_ANY (*_gst_caps_any)

#endif 
